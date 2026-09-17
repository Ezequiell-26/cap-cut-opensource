#include "api/EditorApi.hpp"
#include "render/ExportPresets.hpp"
#include "render/HardwareCapabilities.hpp"
#include "render/TimelineExporter.hpp"
#include <QFileInfo>
#include <QJsonArray>

namespace ccos::api {

QJsonObject EditorApi::inspect(const ccos::project::Project& project) {
    QJsonObject out;
    out.insert(QStringLiteral("name"), project.name());
    out.insert(QStringLiteral("assetCount"), static_cast<int>(project.assets().size()));
    out.insert(QStringLiteral("textLayerCount"), static_cast<int>(project.textLayers().size()));
    out.insert(QStringLiteral("trackCount"), static_cast<int>(project.timeline().tracks().size()));
    QJsonArray assets;
    for (const auto& asset : project.assets()) {
        QJsonObject item;
        item.insert(QStringLiteral("id"), QString::fromStdString(asset.id().toString()));
        item.insert(QStringLiteral("name"), asset.name());
        item.insert(QStringLiteral("path"), asset.path());
        item.insert(QStringLiteral("durationMs"), QJsonValue(static_cast<qint64>(asset.metadata().durationMs)));
        item.insert(QStringLiteral("width"), asset.metadata().width);
        item.insert(QStringLiteral("height"), asset.metadata().height);
        assets.append(item);
    }
    out.insert(QStringLiteral("assets"), assets);
    return out;
}

QJsonObject EditorApi::validate(const ccos::project::Project& project) {
    QJsonObject out;
    QJsonArray errors;
    QJsonArray warnings;
    if (project.name().trimmed().isEmpty()) errors.append(QStringLiteral("Project name is empty"));
    if (project.assets().empty()) warnings.append(QStringLiteral("Project contains no media assets"));
    if (project.timeline().tracks().empty()) warnings.append(QStringLiteral("Timeline contains no tracks"));
    for (const auto& asset : project.assets()) {
        if (asset.path().isEmpty()) errors.append(QStringLiteral("Asset %1 has no path").arg(asset.name()));
        else if (!QFileInfo::exists(asset.path())) warnings.append(QStringLiteral("Missing media: %1").arg(asset.path()));
    }
    out.insert(QStringLiteral("ok"), errors.isEmpty());
    out.insert(QStringLiteral("errors"), errors);
    out.insert(QStringLiteral("warnings"), warnings);
    return out;
}

QJsonObject EditorApi::hardwareCapabilities(const QString& ffmpegExecutable) {
    const auto capabilities = ccos::render::HardwareCapabilitiesProbe::detect(ffmpegExecutable);
    QJsonArray encoders;
    for (const auto& encoder : capabilities.encoders) encoders.append(encoder);

    return QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("encoders"), encoders},
        {QStringLiteral("hasNvidia"), capabilities.hasNvidia},
        {QStringLiteral("hasIntel"), capabilities.hasIntel},
        {QStringLiteral("hasAmd"), capabilities.hasAmd},
        {QStringLiteral("hasVideoToolbox"), capabilities.hasVideoToolbox},
        {QStringLiteral("hasVaapi"), capabilities.hasVaapi},
        {QStringLiteral("preferredH264"), capabilities.preferredH264Encoder()},
        {QStringLiteral("preferredHevc"), capabilities.preferredHevcEncoder()}
    };
}

QJsonObject EditorApi::exportPresets() {
    QJsonArray presets;
    for (const auto& preset : ccos::render::ExportPresetCatalog::all()) {
        const auto& settings = preset.settings;
        presets.append(QJsonObject{
            {QStringLiteral("id"), ccos::render::ExportPresetCatalog::idString(preset.id)},
            {QStringLiteral("name"), preset.name},
            {QStringLiteral("description"), preset.description},
            {QStringLiteral("width"), settings.width},
            {QStringLiteral("height"), settings.height},
            {QStringLiteral("fps"), settings.fps},
            {QStringLiteral("container"), settings.container},
            {QStringLiteral("videoCodec"), settings.videoCodec},
            {QStringLiteral("audioCodec"), settings.audioCodec},
            {QStringLiteral("videoBitrateKbps"), settings.videoBitrateKbps},
            {QStringLiteral("audioBitrateKbps"), settings.audioBitrateKbps}
        });
    }
    return QJsonObject{{QStringLiteral("ok"), true}, {QStringLiteral("presets"), presets}};
}

QJsonObject EditorApi::command(ccos::project::Project& project, const QJsonObject& request, const QString& ffmpegExecutable) {
    const QString operation = request.value(QStringLiteral("op")).toString().trimmed().toLower();
    if (operation == QStringLiteral("inspect")) return inspect(project);
    if (operation == QStringLiteral("validate")) return validate(project);
    if (operation == QStringLiteral("hardware_capabilities") || operation == QStringLiteral("hardware")) {
        return hardwareCapabilities(ffmpegExecutable);
    }
    if (operation == QStringLiteral("export_presets") || operation == QStringLiteral("presets")) {
        return exportPresets();
    }
    if (operation == QStringLiteral("set_project_name")) {
        const QString name = request.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty()) return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("name is required")}};
        project.setName(name);
        return QJsonObject{{QStringLiteral("ok"), true}, {QStringLiteral("name"), project.name()}};
    }
    if (operation == QStringLiteral("export")) {
        const QString output = request.value(QStringLiteral("output")).toString();
        if (output.isEmpty()) return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("output is required")}};

        ccos::render::ExportSettings settings;
        if (request.contains(QStringLiteral("preset"))) {
            ccos::render::ExportPresetId presetId;
            if (!ccos::render::ExportPresetCatalog::fromIdString(request.value(QStringLiteral("preset")).toString(), &presetId)) {
                return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("unknown export preset")}};
            }
            settings = ccos::render::ExportPresetCatalog::settingsFor(presetId);
        }
        if (request.contains(QStringLiteral("width"))) settings.width = request.value(QStringLiteral("width")).toInt();
        if (request.contains(QStringLiteral("height"))) settings.height = request.value(QStringLiteral("height")).toInt();
        if (request.contains(QStringLiteral("fps"))) settings.fps = request.value(QStringLiteral("fps")).toDouble();
        if (request.contains(QStringLiteral("container"))) settings.container = request.value(QStringLiteral("container")).toString();
        if (request.contains(QStringLiteral("videoCodec"))) settings.videoCodec = request.value(QStringLiteral("videoCodec")).toString();
        if (request.contains(QStringLiteral("audioCodec"))) settings.audioCodec = request.value(QStringLiteral("audioCodec")).toString();
        if (request.contains(QStringLiteral("videoBitrateKbps"))) settings.videoBitrateKbps = request.value(QStringLiteral("videoBitrateKbps")).toInt();
        if (request.contains(QStringLiteral("audioBitrateKbps"))) settings.audioBitrateKbps = request.value(QStringLiteral("audioBitrateKbps")).toInt();
        QString error;
        if (!settings.validate(&error)) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
        }
        const bool ok = ccos::render::TimelineExporter::exportContiguousVideo(project, output, settings, ffmpegExecutable, &error);
        QJsonObject result{{QStringLiteral("ok"), ok}, {QStringLiteral("output"), output}};
        if (!ok) result.insert(QStringLiteral("error"), error);
        return result;
    }
    return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("Unknown operation: %1").arg(operation)}};
}

} // namespace ccos::api
