#include "api/EditorApi.hpp"
#include "api/CulturalMediaApi.hpp"
#include "api/OpenMeteoApi.hpp"
#include "graphics/GltfAssetInspector.hpp"
#include "render/ExportPresets.hpp"
#include "render/HardwareCapabilities.hpp"
#include "render/TimelineExporter.hpp"
#include "timeline/TimelineEditor.hpp"
#include <QFileInfo>
#include <QJsonArray>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ccos::api {
namespace {

bool parseNonNegativeIndex(const QJsonObject& request, const QString& key, int* value, QString* error) {
    if (!value || !request.contains(key) || !request.value(key).isDouble()) {
        if (error) *error = QStringLiteral("%1 is required and must be an integer").arg(key);
        return false;
    }
    const double raw = request.value(key).toDouble();
    if (!std::isfinite(raw) || raw < 0.0 || raw > static_cast<double>(std::numeric_limits<int>::max()) ||
        raw != std::floor(raw)) {
        if (error) *error = QStringLiteral("%1 must be a non-negative integer").arg(key);
        return false;
    }
    *value = static_cast<int>(raw);
    return true;
}

QJsonObject editResult(const ccos::timeline::Track& track, int clipIndex) {
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
    return QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("track"), track.name()},
        {QStringLiteral("clipIndex"), clipIndex},
        {QStringLiteral("clipId"), QString::fromStdString(clip.id().toString())},
        {QStringLiteral("start"), clip.start().seconds()},
        {QStringLiteral("duration"), clip.duration().seconds()},
        {QStringLiteral("sourceIn"), clip.sourceIn().seconds()},
        {QStringLiteral("sourceOut"), clip.sourceOut().seconds()}
    };
}

} // namespace

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

    for (std::size_t trackIndex = 0; trackIndex < project.timeline().tracks().size(); ++trackIndex) {
        const auto& track = project.timeline().tracks()[trackIndex];
        for (std::size_t clipIndex = 0; clipIndex < track.clips().size(); ++clipIndex) {
            const auto& clip = track.clips()[clipIndex];
            const QString prefix = QStringLiteral("Track %1 clip %2").arg(static_cast<qulonglong>(trackIndex))
                                                                  .arg(static_cast<qulonglong>(clipIndex));
            const bool assetKnown = std::any_of(project.assets().begin(), project.assets().end(),
                                                [&clip](const auto& asset) { return asset.id() == clip.assetId(); });
            if (!assetKnown) {
                errors.append(prefix + QStringLiteral(" references an unknown asset"));
            }
            if (clip.start() < ccos::core::Time{}) {
                errors.append(prefix + QStringLiteral(" has a negative timeline start"));
            }
            if (clip.duration() <= ccos::core::Time{}) {
                errors.append(prefix + QStringLiteral(" has a non-positive duration"));
            }
            if (clip.sourceIn() < ccos::core::Time{} || clip.sourceOut() <= clip.sourceIn()) {
                errors.append(prefix + QStringLiteral(" has an invalid source range"));
            }
            const double speed = clip.speed();
            if (!std::isfinite(speed) || speed <= 0.0) {
                errors.append(prefix + QStringLiteral(" has an invalid speed"));
            }
            const auto& transform = clip.transform();
            if (!std::isfinite(transform.x) || !std::isfinite(transform.y) ||
                !std::isfinite(transform.scaleX) || !std::isfinite(transform.scaleY) ||
                !std::isfinite(transform.rotation) || !std::isfinite(transform.opacity) ||
                !std::isfinite(transform.cropLeft) || !std::isfinite(transform.cropTop) ||
                !std::isfinite(transform.cropRight) || !std::isfinite(transform.cropBottom)) {
                errors.append(prefix + QStringLiteral(" has non-finite transform values"));
            }
            if (transform.scaleX <= 0.0 || transform.scaleY <= 0.0) {
                errors.append(prefix + QStringLiteral(" has non-positive scale"));
            }
            if (transform.opacity < 0.0 || transform.opacity > 1.0) {
                errors.append(prefix + QStringLiteral(" has opacity outside [0, 1]"));
            }
            if (transform.cropLeft < 0.0 || transform.cropTop < 0.0 ||
                transform.cropRight < 0.0 || transform.cropBottom < 0.0 ||
                transform.cropLeft + transform.cropRight >= 1.0 ||
                transform.cropTop + transform.cropBottom >= 1.0) {
                errors.append(prefix + QStringLiteral(" has invalid crop bounds"));
            }
        }
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
        {QStringLiteral("available"), !capabilities.encoders.isEmpty()},
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

QJsonObject EditorApi::culturalMediaProviders() {
    return QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("providers"), QJsonArray{
            QJsonObject{{QStringLiteral("id"), QStringLiteral("internet_archive")},
                        {QStringLiteral("name"), QStringLiteral("Internet Archive")},
                        {QStringLiteral("auth"), QStringLiteral("none")},
                        {QStringLiteral("media"), QStringLiteral("movies,audio,image")},
                        {QStringLiteral("rights"), QStringLiteral("item-specific")}},
            QJsonObject{{QStringLiteral("id"), QStringLiteral("smithsonian_open_access")},
                        {QStringLiteral("name"), QStringLiteral("Smithsonian Open Access")},
                        {QStringLiteral("auth"), QStringLiteral("api_key")},
                        {QStringLiteral("media"), QStringLiteral("image,3d")},
                        {QStringLiteral("rights"), QStringLiteral("CC0 where explicitly marked; verify record")}},
            QJsonObject{{QStringLiteral("id"), QStringLiteral("met_open_access")},
                        {QStringLiteral("name"), QStringLiteral("The Met Open Access")},
                        {QStringLiteral("auth"), QStringLiteral("none")},
                        {QStringLiteral("media"), QStringLiteral("image")},
                        {QStringLiteral("rights"), QStringLiteral("CC0/public domain image where provided")}},
            QJsonObject{{QStringLiteral("id"), QStringLiteral("europeana")},
                        {QStringLiteral("name"), QStringLiteral("Europeana")},
                        {QStringLiteral("auth"), QStringLiteral("api_key")},
                        {QStringLiteral("media"), QStringLiteral("image,audio,video,metadata")},
                        {QStringLiteral("rights"), QStringLiteral("record-specific rights")}},
            QJsonObject{{QStringLiteral("id"), QStringLiteral("library_of_congress")},
                        {QStringLiteral("name"), QStringLiteral("Library of Congress")},
                        {QStringLiteral("auth"), QStringLiteral("none")},
                        {QStringLiteral("media"), QStringLiteral("image,video,audio,document")},
                        {QStringLiteral("rights"), QStringLiteral("item-specific")}}
        }}
    };
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
    if (operation == QStringLiteral("cultural_media_providers") || operation == QStringLiteral("open_media_providers")) {
        return culturalMediaProviders();
    }

    if (operation == QStringLiteral("geocode") || operation == QStringLiteral("location_search")) {
        const QString query = request.value(QStringLiteral("query")).toString().trimmed();
        if (query.isEmpty()) {
            return QJsonObject{{QStringLiteral("ok"), false},
                               {QStringLiteral("error"), QStringLiteral("query is required")}};
        }
        int count = request.value(QStringLiteral("count")).toInt(10);
        count = std::clamp(count, 1, 100);
        const auto locations = ccos::api::OpenMeteoApi::searchLocations(query, count);
        QJsonArray results;
        for (const auto& location : locations) {
            results.append(QJsonObject{
                {QStringLiteral("name"), location.name},
                {QStringLiteral("country"), location.country},
                {QStringLiteral("countryCode"), location.countryCode},
                {QStringLiteral("admin1"), location.admin1},
                {QStringLiteral("timezone"), location.timezone},
                {QStringLiteral("latitude"), location.latitude},
                {QStringLiteral("longitude"), location.longitude}
            });
        }
        return QJsonObject{{QStringLiteral("ok"), true},
                           {QStringLiteral("query"), query},
                           {QStringLiteral("results"), results}};
    }

    if (operation == QStringLiteral("gltf_inspect") || operation == QStringLiteral("inspect_gltf")) {
        const QString path = request.value(QStringLiteral("path")).toString().trimmed();
        if (path.isEmpty()) {
            return QJsonObject{{QStringLiteral("ok"), false},
                               {QStringLiteral("error"), QStringLiteral("path is required")}};
        }
        const auto info = ccos::graphics::GltfAssetInspector::inspect(path);
        QJsonObject result{
            {QStringLiteral("ok"), info.valid},
            {QStringLiteral("supported"), ccos::graphics::GltfAssetInspector::supported()},
            {QStringLiteral("path"), info.path},
            {QStringLiteral("format"), info.format},
            {QStringLiteral("sceneCount"), info.sceneCount},
            {QStringLiteral("nodeCount"), info.nodeCount},
            {QStringLiteral("meshCount"), info.meshCount},
            {QStringLiteral("materialCount"), info.materialCount},
            {QStringLiteral("imageCount"), info.imageCount},
            {QStringLiteral("animationCount"), info.animationCount},
            {QStringLiteral("skinCount"), info.skinCount}
        };
        if (!info.error.isEmpty()) result.insert(QStringLiteral("error"), info.error);
        return result;
    }

    if (operation == QStringLiteral("timeline_slip") || operation == QStringLiteral("slip")) {
        int trackIndex = -1;
        int clipIndex = -1;
        QString parseError;
        if (!parseNonNegativeIndex(request, QStringLiteral("trackIndex"), &trackIndex, &parseError) ||
            !parseNonNegativeIndex(request, QStringLiteral("clipIndex"), &clipIndex, &parseError)) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), parseError}};
        }
        if (!request.contains(QStringLiteral("sourceDeltaMs")) || !request.value(QStringLiteral("sourceDeltaMs")).isDouble()) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("sourceDeltaMs is required and must be a number")}};
        }
        const auto& tracks = project.timeline().tracks();
        if (trackIndex >= static_cast<int>(tracks.size())) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("trackIndex is out of range")}};
        }
        auto& track = project.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
        if (clipIndex >= static_cast<int>(track.clips().size())) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("clipIndex is out of range")}};
        }
        const double deltaMs = request.value(QStringLiteral("sourceDeltaMs")).toDouble();
        if (!std::isfinite(deltaMs) || deltaMs < static_cast<double>(std::numeric_limits<qint64>::min()) ||
            deltaMs > static_cast<double>(std::numeric_limits<qint64>::max())) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("sourceDeltaMs is outside supported bounds")}};
        }
        const auto delta = ccos::core::Time::fromSeconds(deltaMs / 1000.0);
        if (!ccos::timeline::TimelineEditor::slipClip(track, static_cast<std::size_t>(clipIndex), delta)) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("slip operation rejected by timeline invariants")}};
        }
        return editResult(track, clipIndex);
    }

    if (operation == QStringLiteral("timeline_ripple_delete") || operation == QStringLiteral("ripple_delete")) {
        int trackIndex = -1;
        int clipIndex = -1;
        QString parseError;
        if (!parseNonNegativeIndex(request, QStringLiteral("trackIndex"), &trackIndex, &parseError) ||
            !parseNonNegativeIndex(request, QStringLiteral("clipIndex"), &clipIndex, &parseError)) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), parseError}};
        }
        auto& tracks = project.timeline().tracks();
        if (trackIndex >= static_cast<int>(tracks.size())) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("trackIndex is out of range")}};
        }
        auto& track = tracks[static_cast<std::size_t>(trackIndex)];
        if (clipIndex >= static_cast<int>(track.clips().size())) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("clipIndex is out of range")}};
        }
        if (!ccos::timeline::TimelineEditor::rippleDelete(track, static_cast<std::size_t>(clipIndex))) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("ripple delete was rejected by timeline invariants")}};
        }
        return QJsonObject{{QStringLiteral("ok"), true}, {QStringLiteral("track"), track.name()},
                           {QStringLiteral("remainingClips"), static_cast<int>(track.clips().size())}};
    }

    if (operation == QStringLiteral("set_audio_mix") || operation == QStringLiteral("audio_mix")) {
        int trackIndex = -1;
        int clipIndex = -1;
        QString parseError;
        if (!parseNonNegativeIndex(request, QStringLiteral("trackIndex"), &trackIndex, &parseError) ||
            !parseNonNegativeIndex(request, QStringLiteral("clipIndex"), &clipIndex, &parseError)) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), parseError}};
        }
        const auto& tracks = project.timeline().tracks();
        if (trackIndex >= static_cast<int>(tracks.size())) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("trackIndex is out of range")}};
        }
        auto& track = project.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
        if (clipIndex >= static_cast<int>(track.clips().size())) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("clipIndex is out of range")}};
        }
        if (!request.contains(QStringLiteral("gain")) || !request.value(QStringLiteral("gain")).isDouble()) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("gain is required and must be a number")}};
        }
        const double gain = request.value(QStringLiteral("gain")).toDouble();
        if (!std::isfinite(gain) || gain < 0.0 || gain > 4.0) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("gain must be between 0 and 4")}};
        }
        const bool muted = request.value(QStringLiteral("muted")).toBool(false);
        auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
        clip.setAudioGain(gain);
        clip.setAudioMuted(muted);
        return QJsonObject{
            {QStringLiteral("ok"), true},
            {QStringLiteral("track"), track.name()},
            {QStringLiteral("clipIndex"), clipIndex},
            {QStringLiteral("gain"), clip.audioGain()},
            {QStringLiteral("muted"), clip.audioMuted()}
        };
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
