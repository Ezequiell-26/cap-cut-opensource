#include "project/ProjectSerializer.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <unordered_set>
#include <utility>

namespace ccos::project {
namespace {

constexpr int kCurrentVersion = 6;

QJsonObject encodeTime(const ccos::core::Time& time) {
    return QJsonObject{
        {QStringLiteral("n"), static_cast<qint64>(time.numerator())},
        {QStringLiteral("d"), time.denominator()}
    };
}

ccos::core::Time decodeTime(const QJsonObject& object) {
    const qint64 numerator = object.value(QStringLiteral("n")).toInteger();
    const int denominator = object.value(QStringLiteral("d")).toInt(1);
    return ccos::core::Time(numerator, denominator);
}

bool validateTimeObject(const QJsonObject& object, QString* error, const QString& fieldName) {
    if (!object.contains(QStringLiteral("n")) || !object.contains(QStringLiteral("d"))) {
        if (error) *error = QStringLiteral("Invalid time object: %1").arg(fieldName);
        return false;
    }
    const qint64 numerator = object.value(QStringLiteral("n")).toInteger();
    const int denominator = object.value(QStringLiteral("d")).toInt(0);
    if (denominator <= 0) {
        if (error) *error = QStringLiteral("Invalid time denominator: %1").arg(fieldName);
        return false;
    }
    if (std::abs(static_cast<long double>(numerator)) > 9.0e15L || denominator > 1'000'000'000) {
        if (error) *error = QStringLiteral("Time value outside supported bounds: %1").arg(fieldName);
        return false;
    }
    return true;
}

QJsonObject encodeTransform(const ccos::timeline::TransformState& t) {
    return QJsonObject{
        {QStringLiteral("x"), t.x},
        {QStringLiteral("y"), t.y},
        {QStringLiteral("scaleX"), t.scaleX},
        {QStringLiteral("scaleY"), t.scaleY},
        {QStringLiteral("rotation"), t.rotation},
        {QStringLiteral("opacity"), t.opacity},
        {QStringLiteral("cropLeft"), t.cropLeft},
        {QStringLiteral("cropTop"), t.cropTop},
        {QStringLiteral("cropRight"), t.cropRight},
        {QStringLiteral("cropBottom"), t.cropBottom},
        {QStringLiteral("flipHorizontal"), t.flipHorizontal},
        {QStringLiteral("flipVertical"), t.flipVertical}
    };
}

void decodeTransform(const QJsonObject& object, ccos::timeline::TransformState& transform) {
    transform.x = object.value(QStringLiteral("x")).toDouble(transform.x);
    transform.y = object.value(QStringLiteral("y")).toDouble(transform.y);
    transform.scaleX = object.value(QStringLiteral("scaleX")).toDouble(transform.scaleX);
    transform.scaleY = object.value(QStringLiteral("scaleY")).toDouble(transform.scaleY);
    transform.rotation = object.value(QStringLiteral("rotation")).toDouble(transform.rotation);
    transform.opacity = object.value(QStringLiteral("opacity")).toDouble(transform.opacity);
    transform.cropLeft = object.value(QStringLiteral("cropLeft")).toDouble(transform.cropLeft);
    transform.cropTop = object.value(QStringLiteral("cropTop")).toDouble(transform.cropTop);
    transform.cropRight = object.value(QStringLiteral("cropRight")).toDouble(transform.cropRight);
    transform.cropBottom = object.value(QStringLiteral("cropBottom")).toDouble(transform.cropBottom);
    transform.flipHorizontal = object.value(QStringLiteral("flipHorizontal")).toBool(transform.flipHorizontal);
    transform.flipVertical = object.value(QStringLiteral("flipVertical")).toBool(transform.flipVertical);
}

QJsonObject encodeText(const ccos::text::TextLayer& layer) {
    const auto& style = layer.style();
    return QJsonObject{
        {QStringLiteral("id"), QString::fromStdString(layer.id().toString())},
        {QStringLiteral("text"), layer.text()},
        {QStringLiteral("start"), encodeTime(layer.start())},
        {QStringLiteral("duration"), encodeTime(layer.duration())},
        {QStringLiteral("x"), layer.x()},
        {QStringLiteral("y"), layer.y()},
        {QStringLiteral("family"), style.family},
        {QStringLiteral("size"), style.size},
        {QStringLiteral("color"), style.color},
        {QStringLiteral("bold"), style.bold},
        {QStringLiteral("italic"), style.italic},
        {QStringLiteral("opacity"), style.opacity}
    };
}

bool parseUuid(const QJsonValue& value, ccos::core::Uuid& id, bool required, QString* error,
               const QString& fieldName) {
    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        if (required) {
            if (error) *error = QStringLiteral("Missing UUID field: %1").arg(fieldName);
            return false;
        }
        id = ccos::core::Uuid();
        return true;
    }

    id = ccos::core::Uuid(text.toStdString());
    if (id.isNull()) {
        if (error) *error = QStringLiteral("Invalid UUID in field: %1").arg(fieldName);
        return false;
    }
    return true;
}

bool addUniqueId(const ccos::core::Uuid& id, std::unordered_set<std::string>& ids,
                 QString* error, const QString& kind) {
    const std::string value = id.toString();
    if (!ids.insert(value).second) {
        if (error) *error = QStringLiteral("Duplicate %1 UUID: %2")
            .arg(kind, QString::fromStdString(value));
        return false;
    }
    return true;
}

} // namespace

bool ProjectSerializer::save(const Project& project, const QString& path, QString* error) {
    if (path.isEmpty()) {
        if (error) *error = QStringLiteral("Project output path is empty");
        return false;
    }

    QJsonObject root{
        {QStringLiteral("format"), QStringLiteral("ccos.project")},
        {QStringLiteral("version"), kCurrentVersion},
        {QStringLiteral("id"), QString::fromStdString(project.id().toString())},
        {QStringLiteral("name"), project.name()}
    };

    QJsonArray assets;
    std::unordered_set<std::string> assetIds;
    for (const auto& asset : project.assets()) {
        if (!addUniqueId(asset.id(), assetIds, error, QStringLiteral("asset"))) return false;
        assets.append(QJsonObject{
            {QStringLiteral("id"), QString::fromStdString(asset.id().toString())},
            {QStringLiteral("path"), asset.path()},
            {QStringLiteral("name"), asset.name()},
            {QStringLiteral("durationMs"), static_cast<qint64>(asset.metadata().durationMs)},
            {QStringLiteral("width"), asset.metadata().width},
            {QStringLiteral("height"), asset.metadata().height},
            {QStringLiteral("fps"), asset.metadata().fps},
            {QStringLiteral("videoCodec"), asset.metadata().videoCodec},
            {QStringLiteral("audioCodec"), asset.metadata().audioCodec},
            {QStringLiteral("audioChannels"), asset.metadata().audioChannels},
            {QStringLiteral("sampleRate"), asset.metadata().sampleRate}
        });
    }
    root[QStringLiteral("assets")] = assets;

    QJsonArray tracks;
    std::unordered_set<std::string> clipIds;
    for (const auto& track : project.timeline().tracks()) {
        QJsonObject trackObject{
            {QStringLiteral("type"), track.type() == ccos::timeline::TrackType::Video ? QStringLiteral("video") : QStringLiteral("audio")},
            {QStringLiteral("name"), track.name()}
        };
        QJsonArray clips;
        for (const auto& clip : track.clips()) {
            if (!addUniqueId(clip.id(), clipIds, error, QStringLiteral("clip"))) return false;
            QJsonArray effects;
            for (const auto& effect : clip.effects()) effects.append(effect);
            clips.append(QJsonObject{
                {QStringLiteral("id"), QString::fromStdString(clip.id().toString())},
                {QStringLiteral("assetId"), QString::fromStdString(clip.assetId().toString())},
                {QStringLiteral("start"), encodeTime(clip.start())},
                {QStringLiteral("sourceIn"), encodeTime(clip.sourceIn())},
                {QStringLiteral("sourceOut"), encodeTime(clip.sourceOut())},
                {QStringLiteral("speed"), clip.speed()},
                {QStringLiteral("transform"), encodeTransform(clip.transform())},
                {QStringLiteral("effects"), effects},
                {QStringLiteral("transitionInId"), clip.transitionInId()},
                {QStringLiteral("transitionInDurationMs"), clip.transitionInDurationMs()}
            });
        }
        trackObject[QStringLiteral("clips")] = clips;
        tracks.append(trackObject);
    }
    root[QStringLiteral("timeline")] = tracks;

    QJsonArray texts;
    std::unordered_set<std::string> textIds;
    for (const auto& layer : project.textLayers()) {
        if (!addUniqueId(layer.id(), textIds, error, QStringLiteral("text-layer"))) return false;
        texts.append(encodeText(layer));
    }
    root[QStringLiteral("textLayers")] = texts;

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
    const QByteArray bytes = file.readAll();
    const auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = parseError.errorString();
        return false;
    }

    const auto root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("ccos.project")) {
        if (error) *error = QStringLiteral("Unsupported project format");
        return false;
    }

    const int version = root.value(QStringLiteral("version")).toInt(1);
    if (version < 1 || version > kCurrentVersion) {
        if (error) *error = QStringLiteral("Unsupported project version: %1").arg(version);
        return false;
    }

    ccos::core::Uuid projectId;
    if (!parseUuid(root.value(QStringLiteral("id")), projectId, version >= 6, error, QStringLiteral("project.id"))) return false;
    Project loaded(projectId, root.value(QStringLiteral("name")).toString(QStringLiteral("Untitled Project")));

    std::unordered_set<std::string> assetIds;
    for (const auto& value : root.value(QStringLiteral("assets")).toArray()) {
        const auto object = value.toObject();
        ccos::core::Uuid assetId;
        if (!parseUuid(object.value(QStringLiteral("id")), assetId, version >= 6, error, QStringLiteral("asset.id"))) return false;
        if (!addUniqueId(assetId, assetIds, error, QStringLiteral("asset"))) return false;

        ccos::media::MediaAsset asset(assetId, object.value(QStringLiteral("path")).toString());
        auto& metadata = asset.metadata();
        metadata.durationMs = object.value(QStringLiteral("durationMs")).toInteger();
        metadata.width = object.value(QStringLiteral("width")).toInt();
        metadata.height = object.value(QStringLiteral("height")).toInt();
        metadata.fps = object.value(QStringLiteral("fps")).toDouble();
        metadata.videoCodec = object.value(QStringLiteral("videoCodec")).toString();
        metadata.audioCodec = object.value(QStringLiteral("audioCodec")).toString();
        metadata.audioChannels = object.value(QStringLiteral("audioChannels")).toInt();
        metadata.sampleRate = object.value(QStringLiteral("sampleRate")).toInt();
        asset.setName(object.value(QStringLiteral("name")).toString());
        loaded.addAsset(std::move(asset));
    }

    std::unordered_set<std::string> clipIds;
    if (version >= 2 && root.contains(QStringLiteral("timeline"))) {
        auto& tracks = loaded.timeline().tracks();
        tracks.clear();
        for (const auto& value : root.value(QStringLiteral("timeline")).toArray()) {
            const auto object = value.toObject();
            const auto type = object.value(QStringLiteral("type")).toString() == QStringLiteral("audio")
                ? ccos::timeline::TrackType::Audio : ccos::timeline::TrackType::Video;
            ccos::timeline::Track track(type, object.value(QStringLiteral("name")).toString());

            for (const auto& clipValue : object.value(QStringLiteral("clips")).toArray()) {
                const auto clipObject = clipValue.toObject();
                ccos::core::Uuid clipId;
                if (!parseUuid(clipObject.value(QStringLiteral("id")), clipId, version >= 6, error, QStringLiteral("clip.id"))) return false;
                if (!addUniqueId(clipId, clipIds, error, QStringLiteral("clip"))) return false;

                const QString assetIdText = clipObject.value(QStringLiteral("assetId")).toString().trimmed();
                const ccos::core::Uuid assetId(assetIdText.toStdString());
                if (assetId.isNull() || !assetIds.contains(assetId.toString())) {
                    if (error) *error = QStringLiteral("Clip references missing assetId: %1").arg(assetIdText);
                    return false;
                }

                if (!validateTimeObject(clipObject.value(QStringLiteral("start")).toObject(), error, QStringLiteral("clip.start")) ||
                    !validateTimeObject(clipObject.value(QStringLiteral("sourceIn")).toObject(), error, QStringLiteral("clip.sourceIn")) ||
                    !validateTimeObject(clipObject.value(QStringLiteral("sourceOut")).toObject(), error, QStringLiteral("clip.sourceOut"))) {
                    return false;
                }

                ccos::timeline::Clip clip(clipId, assetId);
                clip.setStart(decodeTime(clipObject.value(QStringLiteral("start")).toObject()));
                clip.setSourceRange(
                    decodeTime(clipObject.value(QStringLiteral("sourceIn")).toObject()),
                    decodeTime(clipObject.value(QStringLiteral("sourceOut")).toObject()));

                if (version >= 3) {
                    clip.setSpeed(clipObject.value(QStringLiteral("speed")).toDouble(1.0));
                    decodeTransform(clipObject.value(QStringLiteral("transform")).toObject(), clip.transform());
                    for (const auto& effect : clipObject.value(QStringLiteral("effects")).toArray()) {
                        clip.addEffect(effect.toString());
                    }
                }
                if (version >= 6) {
                    clip.setTransitionIn(
                        clipObject.value(QStringLiteral("transitionInId")).toString(QStringLiteral("cut")),
                        clipObject.value(QStringLiteral("transitionInDurationMs")).toInteger(0));
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

    std::unordered_set<std::string> textIds;
    if (version >= 4) {
        for (const auto& value : root.value(QStringLiteral("textLayers")).toArray()) {
            const auto object = value.toObject();
            ccos::core::Uuid textId;
            if (!parseUuid(object.value(QStringLiteral("id")), textId, version >= 6, error, QStringLiteral("textLayers[].id"))) return false;
            if (!addUniqueId(textId, textIds, error, QStringLiteral("text-layer"))) return false;

            if (!validateTimeObject(object.value(QStringLiteral("start")).toObject(), error, QStringLiteral("textLayers[].start")) ||
                !validateTimeObject(object.value(QStringLiteral("duration")).toObject(), error, QStringLiteral("textLayers[].duration"))) {
                return false;
            }

            ccos::text::TextLayer layer(textId, object.value(QStringLiteral("text")).toString());
            layer.setStart(decodeTime(object.value(QStringLiteral("start")).toObject()));
            layer.setDuration(decodeTime(object.value(QStringLiteral("duration")).toObject()));
            layer.setPosition(object.value(QStringLiteral("x")).toDouble(0.5), object.value(QStringLiteral("y")).toDouble(0.85));

            auto& style = layer.style();
            style.family = object.value(QStringLiteral("family")).toString(style.family);
            style.size = object.value(QStringLiteral("size")).toDouble(style.size);
            style.color = object.value(QStringLiteral("color")).toString(style.color);
            style.bold = object.value(QStringLiteral("bold")).toBool(style.bold);
            style.italic = object.value(QStringLiteral("italic")).toBool(style.italic);
            style.opacity = object.value(QStringLiteral("opacity")).toDouble(style.opacity);
            loaded.addTextLayer(std::move(layer));
        }
    }

    project = std::move(loaded);
    return true;
}

} // namespace ccos::project
