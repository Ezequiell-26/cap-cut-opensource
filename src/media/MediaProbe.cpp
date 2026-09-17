#include "media/MediaProbe.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace ccos::media {

bool MediaProbe::probe(MediaAsset& asset, const QString& executable, QString* error) {
    QProcess process;
    QStringList args{
        QStringLiteral("-v"), QStringLiteral("error"),
        QStringLiteral("-show_entries"), QStringLiteral("format=duration:stream=index,codec_type,codec_name,width,height,r_frame_rate,channels,sample_rate"),
        QStringLiteral("-of"), QStringLiteral("json"), asset.path()
    };
    process.start(executable, args);
    if (!process.waitForStarted(3000)) {
        if (error) *error = QStringLiteral("Unable to start ffprobe: %1").arg(process.errorString());
        return false;
    }
    if (!process.waitForFinished(10000) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(process.readAllStandardError());
        if (error && error->isEmpty()) *error = QStringLiteral("ffprobe failed");
        return false;
    }

    QJsonParseError parseError{};
    const auto document = QJsonDocument::fromJson(process.readAllStandardOutput(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = parseError.errorString();
        return false;
    }

    const auto root = document.object();
    const auto streams = root.value(QStringLiteral("streams")).toArray();
    for (const auto& value : streams) {
        const auto stream = value.toObject();
        const auto type = stream.value(QStringLiteral("codec_type")).toString();
        if (type == QStringLiteral("video")) {
            auto& meta = asset.metadata();
            meta.videoCodec = stream.value(QStringLiteral("codec_name")).toString();
            meta.width = stream.value(QStringLiteral("width")).toInt();
            meta.height = stream.value(QStringLiteral("height")).toInt();
            const auto rate = stream.value(QStringLiteral("r_frame_rate")).toString();
            const auto slash = rate.indexOf('/');
            if (slash > 0) {
                bool okN = false; bool okD = false;
                const double n = rate.left(slash).toDouble(&okN);
                const double d = rate.mid(slash + 1).toDouble(&okD);
                if (okN && okD && d != 0.0) meta.fps = n / d;
            }
        } else if (type == QStringLiteral("audio")) {
            auto& meta = asset.metadata();
            meta.audioCodec = stream.value(QStringLiteral("codec_name")).toString();
            meta.audioChannels = stream.value(QStringLiteral("channels")).toInt();
            meta.sampleRate = stream.value(QStringLiteral("sample_rate")).toInt();
        }
    }
    const auto duration = root.value(QStringLiteral("format")).toObject().value(QStringLiteral("duration")).toDouble();
    asset.metadata().durationMs = static_cast<std::int64_t>(duration * 1000.0);
    return true;
}
}
