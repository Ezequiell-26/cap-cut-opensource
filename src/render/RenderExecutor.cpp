#include "render/RenderExecutor.hpp"
#include "render/TimelineCompositor.hpp"
#include "render/HardwareCapabilities.hpp"
#include "core/ProcessRunner.hpp"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>

#include <algorithm>

namespace ccos::render {
namespace {

QString resolveVideoCodec(const QString& requested, const QString& executable) {
    const QString trimmed = requested.trimmed();
    if (trimmed != QStringLiteral("auto")) return trimmed;
    return HardwareCapabilitiesProbe::detect(executable).preferredH264Encoder(true);
}

bool samePath(const QString& left, const QString& right) {
    const QString a = QDir::cleanPath(QFileInfo(left).absoluteFilePath());
    const QString b = QDir::cleanPath(QFileInfo(right).absoluteFilePath());
#ifdef Q_OS_WIN
    return QString::compare(a, b, Qt::CaseInsensitive) == 0;
#else
    return a == b;
#endif
}

} // namespace

RenderExecutor::RenderExecutor(QObject* parent) : QObject(parent) {
    process_.setProcessChannelMode(QProcess::MergedChannels);
    timeoutTimer_.setSingleShot(true);
    timeoutTimer_.setInterval(kTotalTimeoutMs);

    connect(&process_, &QProcess::readyRead, this, [this] {
        const QByteArray raw = process_.readAll();
        if (raw.isEmpty()) return;

        const QString text = QString::fromLocal8Bit(raw);
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        static const QRegularExpression progressRe(QStringLiteral("out_time_ms=(\\d+)"));
        for (const auto& line : lines) {
            const auto match = progressRe.match(line);
            if (match.hasMatch()) {
                bool ok = false;
                const qint64 milliseconds = match.captured(1).toLongLong(&ok);
                if (ok) Q_EMIT progress(milliseconds / 1'000'000.0);
            } else if (!line.startsWith(QStringLiteral("progress="))) {
                const QString messageText = line.trimmed();
                if (!messageText.isEmpty()) {
                    appendDiagnostic(messageText);
                    Q_EMIT message(messageText);
                }
            }
        }
    });

    connect(&timeoutTimer_, &QTimer::timeout, this, [this] {
        if (!running()) return;
        appendDiagnostic(QStringLiteral("FFmpeg render exceeded the maximum execution time"));
        process_.kill();
    });

    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && !running()) {
            timeoutTimer_.stop();
            Q_EMIT finished(false, process_.errorString());
        }
    });

    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                timeoutTimer_.stop();
                if (code == 0 && status == QProcess::NormalExit) {
                    Q_EMIT finished(true, {});
                    return;
                }
                const QString diagnostics = QString::fromUtf8(diagnosticBuffer_).trimmed();
                Q_EMIT finished(false, diagnostics.isEmpty()
                    ? QStringLiteral("FFmpeg exited with code %1").arg(code)
                    : diagnostics);
            });
}

void RenderExecutor::appendDiagnostic(const QString& text) {
    const QByteArray line = text.toUtf8();
    if (line.isEmpty() || kDiagnosticLimit <= 0) return;
    if (diagnosticBuffer_.size() >= kDiagnosticLimit) return;

    const qsizetype remaining = kDiagnosticLimit - diagnosticBuffer_.size();
    const qsizetype toAppend = std::min<qsizetype>(remaining, line.size());
    diagnosticBuffer_.append(line.constData(), toAppend);
    if (diagnosticBuffer_.size() < kDiagnosticLimit) diagnosticBuffer_.append('\n');
}

bool RenderExecutor::start(const ccos::project::Project& project, const QString& output,
                           const ExportSettings& settings, const QString& executable) {
    if (running() || output.trimmed().isEmpty()) return false;

    QString error;
    if (!settings.validate(&error)) {
        Q_EMIT finished(false, error);
        return false;
    }
    if (!ccos::core::ProcessRunner::validateExecutable(executable)) {
        error = QStringLiteral("FFmpeg executable not found or not executable: %1").arg(executable);
        Q_EMIT finished(false, error);
        return false;
    }

    QStringList inputs;
    QString filter;
    QString video;
    QString audio;
    if (!TimelineCompositor::build(project, settings, inputs, filter, video, audio, &error)) {
        Q_EMIT finished(false, error);
        return false;
    }
    if (inputs.isEmpty()) {
        Q_EMIT finished(false, QStringLiteral("The timeline contains no media inputs"));
        return false;
    }
    for (const auto& input : inputs) {
        if (samePath(input, output)) {
            Q_EMIT finished(false, QStringLiteral("Output path must not overwrite a timeline input"));
            return false;
        }
    }

    const QFileInfo outputInfo(output);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        Q_EMIT finished(false, QStringLiteral("Unable to create output directory: %1").arg(outputInfo.absolutePath()));
        return false;
    }

    const QString videoCodec = resolveVideoCodec(settings.videoCodec, executable);
    if (videoCodec.isEmpty()) {
        error = QStringLiteral("Unable to resolve the requested video encoder");
        Q_EMIT finished(false, error);
        return false;
    }

    diagnosticBuffer_.clear();
    QStringList args{
        QStringLiteral("-hide_banner"),
        QStringLiteral("-progress"),
        QStringLiteral("pipe:1"),
        QStringLiteral("-nostats"),
        QStringLiteral("-y")
    };
    for (const auto& input : inputs) args << QStringLiteral("-i") << input;
    args << QStringLiteral("-filter_complex") << filter
         << QStringLiteral("-map") << video
         << QStringLiteral("-map") << audio
         << QStringLiteral("-c:v") << videoCodec
         << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(settings.videoBitrateKbps)
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
         << QStringLiteral("-r") << QString::number(settings.fps, 'f', 3)
         << QStringLiteral("-c:a") << settings.audioCodec
         << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(settings.audioBitrateKbps)
         << QStringLiteral("-shortest")
         << output;

    process_.setProcessEnvironment(
        ccos::core::ProcessRunner::sanitizedEnvironment(QProcessEnvironment::systemEnvironment()));
    process_.start(executable, args);
    if (!process_.waitForStarted(kStartupTimeoutMs)) {
        timeoutTimer_.stop();
        Q_EMIT finished(false, process_.errorString());
        return false;
    }
    timeoutTimer_.start();
    return true;
}

void RenderExecutor::cancel() {
    if (!running()) return;
    timeoutTimer_.stop();
    process_.kill();
}

}
