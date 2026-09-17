#include "project/ProjectSerializer.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ccos::project {

bool ProjectSerializer::save(const Project& project, const QString& path, QString* error) {
    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("ccos.project");
    root[QStringLiteral("version")] = 1;
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
        assets.append(item);
    }
    root[QStringLiteral("assets")] = assets;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = file.errorString();
        return false;
    }
    const auto bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        if (error) *error = file.errorString();
        return false;
    }
    file.flush();
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
    project = Project(root.value(QStringLiteral("name")).toString(QStringLiteral("Untitled Project")));
    for (const auto& value : root.value(QStringLiteral("assets")).toArray()) {
        const auto obj = value.toObject();
        ccos::media::MediaAsset asset(obj.value(QStringLiteral("path")).toString());
        auto& metadata = asset.metadata();
        metadata.durationMs = obj.value(QStringLiteral("durationMs")).toInteger();
        metadata.width = obj.value(QStringLiteral("width")).toInt();
        metadata.height = obj.value(QStringLiteral("height")).toInt();
        metadata.fps = obj.value(QStringLiteral("fps")).toDouble();
        project.addAsset(std::move(asset));
    }
    return true;
}
}
