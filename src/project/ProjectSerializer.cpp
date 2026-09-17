#include "project/ProjectSerializer.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace ccos::project {
namespace {
QJsonObject encodeTime(const ccos::core::Time& time) {
    return QJsonObject{{QStringLiteral("n"), static_cast<qint64>(time.numerator())}, {QStringLiteral("d"), time.denominator()}};
}
ccos::core::Time decodeTime(const QJsonObject& object) {
    return ccos::core::Time(object.value(QStringLiteral("n")).toInteger(), object.value(QStringLiteral("d")).toInt(1));
}
QJsonObject encodeTransform(const ccos::timeline::TransformState& t) {
    return QJsonObject{{QStringLiteral("x"), t.x}, {QStringLiteral("y"), t.y}, {QStringLiteral("scaleX"), t.scaleX}, {QStringLiteral("scaleY"), t.scaleY}, {QStringLiteral("rotation"), t.rotation}, {QStringLiteral("opacity"), t.opacity}};
}
void decodeTransform(const QJsonObject& o, ccos::timeline::TransformState& t) {
    t.x = o.value(QStringLiteral("x")).toDouble(t.x); t.y = o.value(QStringLiteral("y")).toDouble(t.y);
    t.scaleX = o.value(QStringLiteral("scaleX")).toDouble(t.scaleX); t.scaleY = o.value(QStringLiteral("scaleY")).toDouble(t.scaleY);
    t.rotation = o.value(QStringLiteral("rotation")).toDouble(t.rotation); t.opacity = o.value(QStringLiteral("opacity")).toDouble(t.opacity);
}
QJsonObject encodeText(const ccos::text::TextLayer& layer) {
    const auto& s = layer.style();
    return QJsonObject{
        {QStringLiteral("id"), QString::fromStdString(layer.id().toString())},
        {QStringLiteral("text"), layer.text()}, {QStringLiteral("start"), encodeTime(layer.start())},
        {QStringLiteral("duration"), encodeTime(layer.duration())}, {QStringLiteral("x"), layer.x()}, {QStringLiteral("y"), layer.y()},
        {QStringLiteral("family"), s.family}, {QStringLiteral("size"), s.size}, {QStringLiteral("color"), s.color},
        {QStringLiteral("bold"), s.bold}, {QStringLiteral("italic"), s.italic}, {QStringLiteral("opacity"), s.opacity}};
}
}

bool ProjectSerializer::save(const Project& project, const QString& path, QString* error) {
    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("ccos.project"); root[QStringLiteral("version")] = 4;
    root[QStringLiteral("id")] = QString::fromStdString(project.id().toString()); root[QStringLiteral("name")] = project.name();

    QJsonArray assets;
    for (const auto& asset : project.assets()) {
        QJsonObject item{{QStringLiteral("id"), QString::fromStdString(asset.id().toString())}, {QStringLiteral("path"), asset.path()}, {QStringLiteral("name"), asset.name()},
                         {QStringLiteral("durationMs"), static_cast<qint64>(asset.metadata().durationMs)}, {QStringLiteral("width"), asset.metadata().width}, {QStringLiteral("height"), asset.metadata().height},
                         {QStringLiteral("fps"), asset.metadata().fps}, {QStringLiteral("videoCodec"), asset.metadata().videoCodec}, {QStringLiteral("audioCodec"), asset.metadata().audioCodec},
                         {QStringLiteral("audioChannels"), asset.metadata().audioChannels}, {QStringLiteral("sampleRate"), asset.metadata().sampleRate}};
        assets.append(item);
    }
    root[QStringLiteral("assets")] = assets;

    QJsonArray tracks;
    for (const auto& track : project.timeline().tracks()) {
        QJsonObject trackObject{{QStringLiteral("type"), track.type() == ccos::timeline::TrackType::Video ? QStringLiteral("video") : QStringLiteral("audio")}, {QStringLiteral("name"), track.name()}};
        QJsonArray clips;
        for (const auto& clip : track.clips()) {
            QJsonArray effects; for (const auto& effect : clip.effects()) effects.append(effect);
            clips.append(QJsonObject{{QStringLiteral("id"), QString::fromStdString(clip.id().toString())}, {QStringLiteral("assetId"), QString::fromStdString(clip.assetId().toString())},
                                     {QStringLiteral("start"), encodeTime(clip.start())}, {QStringLiteral("sourceIn"), encodeTime(clip.sourceIn())}, {QStringLiteral("sourceOut"), encodeTime(clip.sourceOut())},
                                     {QStringLiteral("speed"), clip.speed()}, {QStringLiteral("transform"), encodeTransform(clip.transform())}, {QStringLiteral("effects"), effects}});
        }
        trackObject[QStringLiteral("clips")] = clips; tracks.append(trackObject);
    }
    root[QStringLiteral("timeline")] = tracks;

    QJsonArray texts; for (const auto& layer : project.textLayers()) texts.append(encodeText(layer)); root[QStringLiteral("textLayers")] = texts;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { if (error) *error = file.errorString(); return false; }
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) { if (error) *error = file.errorString(); return false; }
    return true;
}

bool ProjectSerializer::load(Project& project, const QString& path, QString* error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { if (error) *error = file.errorString(); return false; }
    QJsonParseError parseError{}; const auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) { if (error) *error = parseError.errorString(); return false; }
    const auto root = doc.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("ccos.project")) { if (error) *error = QStringLiteral("Unsupported project format"); return false; }
    const int version = root.value(QStringLiteral("version")).toInt(1);
    if (version < 1 || version > 4) { if (error) *error = QStringLiteral("Unsupported project version: %1").arg(version); return false; }

    project = Project(root.value(QStringLiteral("name")).toString(QStringLiteral("Untitled Project")));
    for (const auto& value : root.value(QStringLiteral("assets")).toArray()) {
        const auto obj = value.toObject();
        ccos::media::MediaAsset asset(ccos::core::Uuid(obj.value(QStringLiteral("id")).toString().toStdString()), obj.value(QStringLiteral("path")).toString());
        auto& metadata = asset.metadata(); metadata.durationMs = obj.value(QStringLiteral("durationMs")).toInteger(); metadata.width = obj.value(QStringLiteral("width")).toInt(); metadata.height = obj.value(QStringLiteral("height")).toInt();
        metadata.fps = obj.value(QStringLiteral("fps")).toDouble(); metadata.videoCodec = obj.value(QStringLiteral("videoCodec")).toString(); metadata.audioCodec = obj.value(QStringLiteral("audioCodec")).toString();
        metadata.audioChannels = obj.value(QStringLiteral("audioChannels")).toInt(); metadata.sampleRate = obj.value(QStringLiteral("sampleRate")).toInt(); project.addAsset(std::move(asset));
    }

    if (version >= 2 && root.contains(QStringLiteral("timeline"))) {
        auto& tracks = project.timeline().tracks(); tracks.clear();
        for (const auto& value : root.value(QStringLiteral("timeline")).toArray()) {
            const auto object = value.toObject(); const auto type = object.value(QStringLiteral("type")).toString() == QStringLiteral("audio") ? ccos::timeline::TrackType::Audio : ccos::timeline::TrackType::Video;
            ccos::timeline::Track track(type, object.value(QStringLiteral("name")).toString());
            for (const auto& clipValue : object.value(QStringLiteral("clips")).toArray()) {
                const auto clipObject = clipValue.toObject();
                ccos::timeline::Clip clip(ccos::core::Uuid(clipObject.value(QStringLiteral("id")).toString().toStdString()), ccos::core::Uuid(clipObject.value(QStringLiteral("assetId")).toString().toStdString()));
                clip.setStart(decodeTime(clipObject.value(QStringLiteral("start")).toObject()));
                clip.setSourceRange(decodeTime(clipObject.value(QStringLiteral("sourceIn")).toObject()), decodeTime(clipObject.value(QStringLiteral("sourceOut")).toObject()));
                if (version >= 3) {
                    clip.setSpeed(clipObject.value(QStringLiteral("speed")).toDouble(1.0)); decodeTransform(clipObject.value(QStringLiteral("transform")).toObject(), clip.transform());
                    for (const auto& effect : clipObject.value(QStringLiteral("effects")).toArray()) clip.addEffect(effect.toString());
                }
                track.addClip(clip);
            }
            tracks.push_back(std::move(track));
        }
        if (tracks.empty()) { tracks.emplace_back(ccos::timeline::TrackType::Video); tracks.emplace_back(ccos::timeline::TrackType::Audio); }
    }

    if (version >= 4) {
        for (const auto& value : root.value(QStringLiteral("textLayers")).toArray()) {
            const auto o = value.toObject();
            ccos::text::TextLayer layer(o.value(QStringLiteral("text")).toString());
            layer.setStart(decodeTime(o.value(QStringLiteral("start")).toObject())); layer.setDuration(decodeTime(o.value(QStringLiteral("duration")).toObject()));
            layer.setPosition(o.value(QStringLiteral("x")).toDouble(0.5), o.value(QStringLiteral("y")).toDouble(0.85));
            auto& s = layer.style(); s.family = o.value(QStringLiteral("family")).toString(s.family); s.size = o.value(QStringLiteral("size")).toDouble(s.size); s.color = o.value(QStringLiteral("color")).toString(s.color);
            s.bold = o.value(QStringLiteral("bold")).toBool(s.bold); s.italic = o.value(QStringLiteral("italic")).toBool(s.italic); s.opacity = o.value(QStringLiteral("opacity")).toDouble(s.opacity);
            project.addTextLayer(std::move(layer));
        }
    }
    return true;
}
}
