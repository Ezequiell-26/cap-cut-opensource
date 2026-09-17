#include "web/WebFfmpegRenderer.hpp"

#include "render/TimelineCompositor.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <mutex>
#include <unordered_map>
#include <utility>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

namespace ccos::web {
namespace {

struct CallbackState {
    WebFfmpegRenderer::Callback callback;
};

std::mutex g_callbackMutex;
int g_nextCallbackId = 1;
std::unordered_map<int, CallbackState> g_callbacks;

int registerCallback(WebFfmpegRenderer::Callback callback) {
    if (!callback) return 0;
    std::lock_guard lock(g_callbackMutex);
    const int id = g_nextCallbackId++;
    g_callbacks.emplace(id, CallbackState{std::move(callback)});
    return id;
}

void completeCallback(int callbackId, bool ok, const QString& message) {
    WebFfmpegRenderer::Callback callback;
    {
        std::lock_guard lock(g_callbackMutex);
        const auto it = g_callbacks.find(callbackId);
        if (it == g_callbacks.end()) return;
        callback = std::move(it->second.callback);
        g_callbacks.erase(it);
    }
    if (callback) callback(ok, message);
}

QString browserSafeCodec(QString codec) {
    codec = codec.trimmed();
    const QString lower = codec.toLower();
    if (codec.isEmpty() || lower == QStringLiteral("auto") ||
        lower.contains(QStringLiteral("nvenc")) ||
        lower.contains(QStringLiteral("qsv")) ||
        lower.contains(QStringLiteral("amf")) ||
        lower.contains(QStringLiteral("videotoolbox")) ||
        lower.contains(QStringLiteral("vaapi"))) {
        return QStringLiteral("libx264");
    }
    static const QRegularExpression safe(QStringLiteral("^[A-Za-z0-9._-]{1,64}$"));
    return safe.match(codec).hasMatch() ? codec : QStringLiteral("libx264");
}

QString browserSafeAudioCodec(QString codec) {
    codec = codec.trimmed();
    static const QRegularExpression safe(QStringLiteral("^[A-Za-z0-9._-]{1,64}$"));
    if (codec.isEmpty() || !safe.match(codec).hasMatch()) return QStringLiteral("aac");
    return codec;
}

QString browserSafeExtension(QString container) {
    container = container.trimmed().toLower();
    if (container == QStringLiteral("webm")) return QStringLiteral("webm");
    if (container == QStringLiteral("mkv")) return QStringLiteral("mkv");
    return QStringLiteral("mp4");
}

} // namespace

#ifdef __EMSCRIPTEN__

extern "C" EMSCRIPTEN_KEEPALIVE void ccos_web_ffmpeg_complete(int callbackId, int ok, const char* message) {
    completeCallback(callbackId, ok != 0, QString::fromUtf8(message == nullptr ? "" : message));
}

EM_JS(void, ccos_web_ffmpeg_start, (const char* specPtr, int callbackId), {
    const spec = JSON.parse(UTF8ToString(specPtr));

    if (!globalThis.__ccosFFmpegState) {
        globalThis.__ccosFFmpegState = { active: null, ffmpeg: null, readyPromise: null };
    }
    const state = globalThis.__ccosFFmpegState;

    const complete = (ok, message) => {
        if (!state.active || state.active.id !== callbackId || state.active.finished) return;
        state.active.finished = true;
        try {
            ccall('ccos_web_ffmpeg_complete', 'void', ['number', 'number', 'string'], [callbackId, ok ? 1 : 0, String(message || '')]);
        } catch (e) {
            console.error('CCOS FFmpeg callback failed', e);
        }
        state.active = null;
    };

    const toBlobURL = async (url, mimeType) => {
        const response = await fetch(url, { cache: 'no-store' });
        if (!response.ok) throw new Error(`Unable to load ${url}: HTTP ${response.status}`);
        const blob = await response.blob();
        return URL.createObjectURL(new Blob([blob], { type: mimeType }));
    };

    const loadFFmpeg = async () => {
        if (state.ffmpeg && state.ffmpeg.loaded) return state.ffmpeg;
        if (state.readyPromise) return state.readyPromise;

        state.readyPromise = (async () => {
            const moduleURL = new URL('vendor/ffmpeg/index.js', document.baseURI).href;
            const coreBase = new URL('vendor/ffmpeg-core/', document.baseURI).href;
            const ffmpegModule = await import(moduleURL);
            const ffmpeg = new ffmpegModule.FFmpeg();
            ffmpeg.on('log', ({ message }) => {
                if (state.active && !state.active.finished) {
                    state.active.lastLog = message || '';
                }
            });
            state.ffmpeg = ffmpeg;
            await ffmpeg.load({
                coreURL: await toBlobURL(coreBase + 'ffmpeg-core.js', 'text/javascript'),
                wasmURL: await toBlobURL(coreBase + 'ffmpeg-core.wasm', 'application/wasm')
            });
            return ffmpeg;
        })().catch((error) => {
            state.readyPromise = null;
            throw error;
        });

        return state.readyPromise;
    };

    const sanitizeName = (name, fallback) => {
        const base = String(name || fallback).split('/').pop().split('\\').pop();
        return base.replace(/[^A-Za-z0-9._-]/g, '_').slice(0, 128) || fallback;
    };

    const download = (data, name, mime) => {
        const blob = new Blob([data], { type: mime });
        const url = URL.createObjectURL(blob);
        const anchor = document.createElement('a');
        anchor.href = url;
        anchor.download = name;
        anchor.rel = 'noopener';
        document.body.appendChild(anchor);
        anchor.click();
        anchor.remove();
        setTimeout(() => URL.revokeObjectURL(url), 30000);
    };

    state.active = { id: callbackId, finished: false, lastLog: '' };

    (async () => {
        try {
            const ffmpeg = await loadFFmpeg();
            const inputNames = [];
            for (let i = 0; i < spec.inputs.length; i += 1) {
                const sourcePath = String(spec.inputs[i]);
                if (!FS.analyzePath(sourcePath).exists) {
                    throw new Error(`Input media is unavailable in browser storage: ${sourcePath}`);
                }
                const name = `ccos_input_${i}_${sanitizeName(sourcePath, `input_${i}.bin`)}`;
                await ffmpeg.writeFile(name, FS.readFile(sourcePath));
                inputNames.push(name);
            }

            const outputExt = sanitizeName(spec.extension || 'mp4', 'mp4');
            const outputName = `ccos-export-${Date.now()}.${outputExt}`;
            const args = ['-hide_banner'];
            inputNames.forEach((name) => { args.push('-i', name); });
            args.push('-filter_complex', String(spec.filterComplex || ''));
            args.push('-map', String(spec.videoMap || '0:v:0'));
            args.push('-map', String(spec.audioMap || '0:a:0'));
            args.push('-c:v', String(spec.videoCodec || 'libx264'));
            args.push('-b:v', `${Number(spec.videoBitrateKbps || 8000)}k`);
            args.push('-pix_fmt', 'yuv420p');
            args.push('-r', Number(spec.fps || 30).toFixed(3));
            args.push('-c:a', String(spec.audioCodec || 'aac'));
            args.push('-b:a', `${Number(spec.audioBitrateKbps || 192)}k`);
            args.push('-shortest');
            if (outputExt === 'mp4') args.push('-movflags', '+faststart');
            args.push(outputName);

            const exitCode = await ffmpeg.exec(args, 60 * 60 * 1000);
            if (!state.active || state.active.id !== callbackId) return;
            if (exitCode !== 0) {
                throw new Error(state.active.lastLog || `FFmpeg exited with code ${exitCode}`);
            }

            const outputData = await ffmpeg.readFile(outputName);
            const mime = outputExt === 'webm' ? 'video/webm' : (outputExt === 'mkv' ? 'video/x-matroska' : 'video/mp4');
            download(outputData, sanitizeName(spec.outputName || outputName, outputName), mime);
            await ffmpeg.deleteFile(outputName).catch(() => {});
            for (const name of inputNames) await ffmpeg.deleteFile(name).catch(() => {});
            complete(true, 'Browser export completed and the file download was started.');
        } catch (error) {
            complete(false, error instanceof Error ? error.message : String(error));
        }
    })();
});

EM_JS(void, ccos_web_ffmpeg_cancel, (), {
    const state = globalThis.__ccosFFmpegState;
    if (!state || !state.active) return;
    const callbackId = state.active.id;
    state.active.finished = true;
    if (state.ffmpeg) {
        try { state.ffmpeg.terminate(); } catch (e) { console.warn('CCOS FFmpeg terminate failed', e); }
    }
    state.ffmpeg = null;
    state.readyPromise = null;
    try {
        ccall('ccos_web_ffmpeg_complete', 'void', ['number', 'number', 'string'], [callbackId, 0, 'Export cancelled by user.']);
    } catch (e) {
        console.error('CCOS FFmpeg cancel callback failed', e);
    }
    state.active = null;
});

#endif

bool WebFfmpegRenderer::exportTimeline(const ccos::project::Project& project,
                                       const ccos::render::ExportSettings& settings,
                                       Callback callback) {
    QString error;
    if (!settings.validate(&error)) {
        if (callback) callback(false, error);
        return false;
    }

    QStringList inputs;
    QString filterComplex;
    QString videoMap;
    QString audioMap;
    if (!ccos::render::TimelineCompositor::build(project, settings, inputs, filterComplex, videoMap, audioMap, &error)) {
        if (callback) callback(false, error);
        return false;
    }
    if (inputs.isEmpty()) {
        if (callback) callback(false, QStringLiteral("The timeline contains no media inputs"));
        return false;
    }

#ifdef __EMSCRIPTEN__
    const int callbackId = registerCallback(std::move(callback));
    if (callbackId == 0) return false;

    QJsonObject spec;
    QJsonArray inputArray;
    for (const QString& input : inputs) inputArray.append(input);
    spec.insert(QStringLiteral("inputs"), inputArray);
    spec.insert(QStringLiteral("filterComplex"), filterComplex);
    spec.insert(QStringLiteral("videoMap"), videoMap);
    spec.insert(QStringLiteral("audioMap"), audioMap);
    spec.insert(QStringLiteral("videoCodec"), browserSafeCodec(settings.videoCodec));
    spec.insert(QStringLiteral("audioCodec"), browserSafeAudioCodec(settings.audioCodec));
    spec.insert(QStringLiteral("videoBitrateKbps"), settings.videoBitrateKbps);
    spec.insert(QStringLiteral("audioBitrateKbps"), settings.audioBitrateKbps);
    spec.insert(QStringLiteral("fps"), settings.fps);
    spec.insert(QStringLiteral("extension"), browserSafeExtension(settings.container));
    spec.insert(QStringLiteral("outputName"), QStringLiteral("ccos-export.%1").arg(browserSafeExtension(settings.container)));

    const QByteArray serialized = QJsonDocument(spec).toJson(QJsonDocument::Compact);
    ccos_web_ffmpeg_start(serialized.constData(), callbackId);
    return true;
#else
    if (callback) callback(false, QStringLiteral("WebFfmpegRenderer is only available in WebAssembly builds"));
    return false;
#endif
}

void WebFfmpegRenderer::cancel() {
#ifdef __EMSCRIPTEN__
    ccos_web_ffmpeg_cancel();
#endif
}

bool WebFfmpegRenderer::available() noexcept {
#ifdef __EMSCRIPTEN__
    return true;
#else
    return false;
#endif
}

} // namespace ccos::web
