#include "project/ProjectSerializer.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace ccos::project {
namespace {
QJsonObject encodeTime(const ccos::core::Time& time) {
    return QJsonObject{{QStringLiteral("n"), static_cast<qint64>(time.numerator())},
                       {QStringLiteral("d"), time.denominator()}};
}

ccos::core::Time decodeTime(const QJsonObject& object) {
    return ccos::core::Time(object.value(QStringLiteral("n")).toInteger(),
                            object.value(QStringLiteral("d")).toInt(1));
}

QJsonObject encodeTransform(const ccos::timeline::TransformState& t) {
    return QJsonObject{{QStringLiteral("x"), t.x}, {QStringLiteral("y"), t.y},
                       {QStringLiteral("scaleX"), t.scaleX}, {QStringLiteral("scaleY"), t.scaleY},
                       {QStringLiteral("rotation"), t.rotation}, {QStringLiteral("opacity"), t.opacity}};
}

void decodeTransform(const QJsonObject& o, ccos::timeline::TransformState& t) {
    t.x = o.value(QStringLiteral("x")).toDouble(t.x);
    t.y = o.value(QStringLiteral("y")).toDouble(t.y);
    t.scaleX = o.value(QStringLiteral("scaleX")).toDouble(t.scaleX);
    t.scaleY = o.value(QStringLiteral("scaleY")).toDouble(t.scaleY);
    t.rotation = o.value(QStringLiteral("rotation")).toDouble(t.rotation);
    t.opacity = o.value(QStringLiteral("opacity")).toDouble(t.opacity);
}
}

bool ProjectSerializer::save(const Project& project, const QString& path, QString* error) {
    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("ccos.project");
    root[QStringLiteral("version")] = 3;
    root[QStringLiteral("id")] = QString::fromStdString(project.id().toString());
    root[QStringLiteral("name")] = project.name();

    QJsonArray assets;
    for (const auto& asset : project.assets()) {
        QJsonObject item;
        item[QStringLiteral("id")] = QString::fromStdString(asset.id().toString());
        item[QStringLiteral("path")] = asset.path();
        item[QStringLiteral("name")] = asset.name();
        item[QStringLiteral("durationMs")] = static_cast<qint64>(asset.metadata().durationMs);
        item[QStringLiteral("width")] = asset.metadata().width;
        item[QStringLiteral("height")] = asset.metadata().height;
        item[QStringLiteral("fps")] = asset.metadata().fps;
        item[QStringLiteral("videoCodec")] = asset.metadata().videoCodec;
        item[QStringLiteral("audioCodec")] = asset.metadata().audioCodec;
        item[QStringLiteral("audioChannels")] = asset.metadata().audioChannels;
        item[QStringLiteral("sampleRate")] = asset.metadata().sampleRate;
        assets.append(item);
    }
    root[QStringLiteral("assets")] = assets;

    QJsonArray tracks;
    for (const auto& track : project.timeline().tracks()) {
        QJsonObject trackObject;
        trackObject[QStringLiteral("type")] = track.type() == ccos::timeline::TrackType::Video ? QStringLiteral("video") : QStringLiteral("audio");
        trackObject[QStringLiteral("name")] = track.name();
        QJsonArray clips;
        for (const auto& clip : track.clips()) {
            QJsonObject clipObject;
            clipObject[QStringLiteral("id")] = QString::fromStdString(clip.id().toString());
            clipObject[QStringLiteral("assetId")] = QString::fromStdString(clip.assetId().toString());
            clipObject[QStringLiteral("start")] = encodeTime(clip.start());
            clipObject[QStringLiteral("sourceIn")] = encodeTime(clip.sourceIn());
            clipObject[QStringLiteral("sourceOut")] = encodeTime(clip.sourceOut());
            clipObject[QStringLiteral("speed")] = clip.speed();
            clipObject[QStringLiteral("transform")] = encodeTransform(clip.transform());
            QJsonArray effectArray;
            for (const auto& effect : clip.effects()) effectArray.append(effect);
            clipObject[QStringLiteral("effects")] = effectArray;
            clips.append(clipObject);
        }
        trackObject[QStringLiteral("clips")] = clips;
        tracks.append(trackObject);
    }
    root[QStringLiteral("timeline")] = tracks;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool ProjectSerializer::load(Project& project, const QString& path, QString* error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    QJsonParseError parseError{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = parseError.errorString();
        return false;
    }
    const auto root = doc.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("ccos.project")) {
        if (error) *error = QStringLiteral("Unsupported project format");
        return false;
    }
    const int version = root.value(QStringLiteral("version")).toInt(1);
    if (version < 1 || version > 3) {
        if (error) *error = QStringLiteral("Unsupported project version: %1").arg(version);
        return false;
    }

    project = Project(root.value(QStringLiteral("name")).toString(QStringLiteral("Untitled Project")));
    for (const auto& value : root.value(QStringLiteral("assets")).toArray()) {
        const auto obj = value.toObject();
        ccos::media::MediaAsset asset(ccos::core::Uuid(obj.value(QStringLiteral("id")).toString().toStdString()),
                                      obj.value(QStringLiteral("path")).toString());
        auto& metadata = asset.metadata();
        metadata.durationMs = obj.value(QStringLiteral("durationMs")).toInteger();
        metadata.width = obj.value(QStringLiteral("width")).toInt();
        metadata.height = obj.value(QStringLiteral("height")).toInt();
        metadata.fps = obj.value(QStringLiteral("fps")).toDouble();
        metadata.videoCodec = obj.value(QStringLiteral("videoCodec")).toString();
        metadata.audioCodec = obj.value(QStringLiteral("audioCodec")).toString();
        metadata.audioChannels = obj.value(QStringLiteral("audioChannels")).toInt();
        metadata.sampleRate = obj.value(QStringLiteral("sampleRate")).toInt();
        project.addAsset(std::move(asset));
    }

    if (version >= 2 && root.contains(QStringLiteral("timeline"))) {
        auto& tracks = project.timeline().tracks();
        tracks.clear();
        for (const auto& value : root.value(QStringLiteral("timeline")).toArray()) {
            const auto object = value.toObject();
            const auto type = object.value(QStringLiteral("type")).toString() == QStringLiteral("audio")
                ? ccos::timeline::TrackType::Audio : ccos::timeline::TrackType::Video;
            ccos::timeline::Track track(type, object.value(QStringLiteral("name")).toString());
            for (const auto& clipValue : object.value(QStringLiteral("clips")).toArray()) {
                const auto clipObject = clipValue.toObject();
                ccos::timeline::Clip clip(
                    ccos::core::Uuid(clipObject.value(QStringLiteral("id")).toString().toStdString()),
                    ccos::core::Uuid(clipObject.value(QStringLiteral("assetId")).toString().toStdString()));
                clip.setStart(decodeTime(clipObject.value(QStringLiteral("start")).toObject()));
                clip.setSourceRange(decodeTime(clipObject.value(QStringLiteral("sourceIn")).toObject()),
                                    decodeTime(clipObject.value(QStringLiteral("sourceOut")).toObject()));
                if (version >= 3) {
                    clip.setSpeed(clipObject.value(QStringLiteral("speed")).toDouble(1.0));
                    decodeTransform(clipObject.value(QStringLiteral("transform")).toObject(), clip.transform());
                    for (const auto& effect : clipObject.value(QStringLiteral("effects")).toArray()) clip.addEffect(effect.toString());
                }
                track.addClip(clip);
            }
            tracks.push_back(std::move(track));
        }
        if (tracks.empty()) {
            tracks.emplace_back(ccos::timeline::TrackType::Video);
            tracks.emplace_back(ccos::timeline::TrackType::Audio);
        }
    }
    return true;
}
}
