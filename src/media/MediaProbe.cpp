#include "media/MediaProbe.hpp"
#include "core/ProcessRunner.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>
#include <cstdint>

namespace ccos::media {

bool MediaProbe::probe(MediaAsset& asset, const QString& executable, QString* error) {
    if (asset.path().trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Media path is empty");
        return false;
    }

    const QStringList args{
        QStringLiteral("-v"), QStringLiteral("error"), QStringLiteral("-show_entries"),
        QStringLiteral("format=duration:stream=index,codec_type,codec_name,width,height,r_frame_rate,channels,sample_rate"),
        QStringLiteral("-of"), QStringLiteral("json"), asset.path()};

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.timeout = std::chrono::seconds(10);
    config.startupTimeout = std::chrono::seconds(3);
    config.maxOutputSize = 4 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::High;
    config.sanitizeEnvironment = true;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) {
        if (error) *error = result.errorMessage();
        return false;
    }
    if (result.outputTruncated) {
        if (error) *error = QStringLiteral("ffprobe output exceeded the metadata limit");
        return false;
    }

    QJsonParseError parseError{};
    const auto document = QJsonDocument::fromJson(result.standardOutput, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = parseError.errorString();
        return false;
    }

    const auto root = document.object();
    const auto streams = root.value(QStringLiteral("streams")).toArray();
    constexpr int kMaxVideoDimension = 16384;
    constexpr int kMaxAudioChannels = 64;
    constexpr int kMaxSampleRate = 768000;
    constexpr double kMaxFps = 240.0;

    for (const auto& value : streams) {
        const auto stream = value.toObject();
        const auto type = stream.value(QStringLiteral("codec_type")).toString();
        if (type == QStringLiteral("video")) {
            const int width = stream.value(QStringLiteral("width")).toInt();
            const int height = stream.value(QStringLiteral("height")).toInt();
            if (width < 0 || width > kMaxVideoDimension || height < 0 || height > kMaxVideoDimension) {
                if (error) *error = QStringLiteral("ffprobe returned unsupported video dimensions");
                return false;
            }

            auto& meta = asset.metadata();
            meta.videoCodec = stream.value(QStringLiteral("codec_name")).toString();
            meta.width = width;
            meta.height = height;

            const QString rate = stream.value(QStringLiteral("r_frame_rate")).toString();
            const int slash = rate.indexOf(QLatin1Char('/'));
            if (slash > 0) {
                bool okN = false;
                bool okD = false;
                const double numerator = rate.left(slash).toDouble(&okN);
                const double denominator = rate.mid(slash + 1).toDouble(&okD);
                if (okN && okD && std::isfinite(numerator) && std::isfinite(denominator) && denominator != 0.0) {
                    const double fps = numerator / denominator;
                    if (!std::isfinite(fps) || fps < 0.0 || fps > kMaxFps) {
                        if (error) *error = QStringLiteral("ffprobe returned unsupported frame rate");
                        return false;
                    }
                    meta.fps = fps;
                }
            }
        } else if (type == QStringLiteral("audio")) {
            const int channels = stream.value(QStringLiteral("channels")).toInt();
            const int sampleRate = stream.value(QStringLiteral("sample_rate")).toInt();
            if (channels < 0 || channels > kMaxAudioChannels || sampleRate < 0 || sampleRate > kMaxSampleRate) {
                if (error) *error = QStringLiteral("ffprobe returned unsupported audio metadata");
                return false;
            }

            auto& meta = asset.metadata();
            meta.audioCodec = stream.value(QStringLiteral("codec_name")).toString();
            meta.audioChannels = channels;
            meta.sampleRate = sampleRate;
        }
    }

    const double duration = root.value(QStringLiteral("format")).toObject()
                                .value(QStringLiteral("duration")).toDouble();
    if (!std::isfinite(duration) || duration < 0.0) {
        if (error) *error = QStringLiteral("ffprobe returned invalid media duration");
        return false;
    }
    constexpr long double kMaxDurationMs = 9.0e15L;
    const long double durationMs = static_cast<long double>(duration) * 1000.0L;
    if (!std::isfinite(durationMs) || durationMs > kMaxDurationMs) {
        if (error) *error = QStringLiteral("ffprobe returned media duration outside supported bounds");
        return false;
    }
    asset.metadata().durationMs = static_cast<std::int64_t>(durationMs);
    return true;
}
}
