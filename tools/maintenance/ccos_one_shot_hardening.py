from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def replace_once(path: str, old: str, new: str) -> None:
    target = ROOT / path
    text = target.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected exactly one match for replacement, found {count}")
    target.write_text(text.replace(old, new), encoding="utf-8")


def append_if_missing(path: str, marker: str, text: str) -> None:
    target = ROOT / path
    current = target.read_text(encoding="utf-8")
    if marker in current:
        return
    target.write_text(current + text, encoding="utf-8")


def write_if_missing(path: str, content: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists():
        target.write_text(content, encoding="utf-8")


def main() -> None:
    # P0 compiler repairs.
    replace_once(
        "src/api/CulturalMediaApi.cpp",
        "        const int count = std::min(safeLimit, ids.size());",
        "        const int count = static_cast<int>(std::min<qsizetype>(\n            static_cast<qsizetype>(safeLimit), ids.size()));",
    )
    replace_once(
        "src/api/OpenMeteoApi.cpp",
        "#include <QUrl>\n",
        "#include <QUrl>\n#include <QUrlQuery>\n",
    )
    replace_once(
        "src/api/AssetsApi.cpp",
        "    QString familyParam = fontFamily.replace(\" \", \"+\");",
        "    QString familyParam = fontFamily;\n    familyParam.replace(\" \", \"+\");",
    )
    replace_once(
        "src/api/AssetsApi.cpp",
        "    QJsonObject colorObj = obj[\"color\"].toString();\n    // Parse dominant color from hex string\n",
        "    // Parse dominant color from the provider's hex string.\n",
    )
    replace_once(
        "src/api/PremiumStockApi.cpp",
        "[reply, callback, this]() {",
        "[reply, callback, activity, this]() {",
    )

    # Make web and desktop Qt component discovery honest and deterministic.
    replace_once(
        "CMakeLists.txt",
        "find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets Multimedia MultimediaWidgets Concurrent)\n",
        "if(EMSCRIPTEN)\n    find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets Multimedia MultimediaWidgets)\nelse()\n    find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets Multimedia MultimediaWidgets Concurrent)\nendif()\n",
    )
    replace_once(
        "CMakeLists.txt",
        "target_link_libraries(ccos_core PUBLIC Qt6::Core Qt6::Gui Qt6::Network Qt6::Concurrent)\n",
        "target_link_libraries(ccos_core PUBLIC Qt6::Core Qt6::Gui Qt6::Network)\nif(NOT EMSCRIPTEN)\n    target_link_libraries(ccos_core PUBLIC Qt6::Concurrent)\nendif()\n",
    )
    replace_once(
        "cmake/CCOSWebAssembly.cmake",
        "    # Qt 6.8's WebAssembly package does not ship Qt Concurrent. Keep the\n    # shared top-level component list intact for desktop and provide a\n    # zero-library compatibility target for the WASM source subset, which\n    # excludes the native ProcessRunner/JobSystem/PipelineGraph sources.\n    set(Qt6Concurrent_DIR\n        \"${CMAKE_CURRENT_SOURCE_DIR}/cmake/wasm-qt-shims/Qt6Concurrent\"\n        CACHE PATH \"CCOS WebAssembly Qt Concurrent compatibility package\" FORCE)\n",
        "    # WebAssembly intentionally omits Qt Concurrent: the browser target\n    # uses the browser-native async path and never compiles the native job/process\n    # implementations that depend on Qt Concurrent.\n",
    )

    # Improve Google Fonts result bounding while keeping API behavior stable.
    old_fonts = '''    QByteArray data = reply->readAll();\n    reply->deleteLater();\n\n    return parseFontsResponse(QJsonDocument::fromJson(data));\n'''
    new_fonts = '''    QByteArray data = reply->readAll();\n    reply->deleteLater();\n\n    QVector<FontInfo> results = parseFontsResponse(QJsonDocument::fromJson(data));\n    if (maxResults <= 0) return {};\n    if (maxResults < results.size()) results.resize(maxResults);\n    return results;\n'''
    replace_once("src/api/AssetsApi.cpp", old_fonts, new_fonts)

    # New production-grade domain primitives. They are intentionally independent
    # from the GUI so they can be wired into native and web frontends incrementally.
    write_if_missing(
        "src/audio/AudioMeter.hpp",
        r'''#pragma once\n\n#include <cstddef>\n#include <span>\n\nnamespace ccos::audio {\n\nstruct MeterReading {\n    float peak = 0.0F;\n    float rms = 0.0F;\n    float peakDbfs = -100.0F;\n    float rmsDbfs = -100.0F;\n};\n\nclass AudioMeter final {\npublic:\n    [[nodiscard]] static MeterReading analyze(std::span<const float> interleaved,\n                                              std::size_t channels = 1) noexcept;\n    [[nodiscard]] static float toDbfs(float linear) noexcept;\n};\n\n} // namespace ccos::audio\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/audio/AudioMeter.cpp",
        r'''#include "audio/AudioMeter.hpp"\n\n#include <algorithm>\n#include <cmath>\n#include <limits>\n\nnamespace ccos::audio {\n\nfloat AudioMeter::toDbfs(float linear) noexcept {\n    if (!(linear > 0.0F) || !std::isfinite(linear)) return -100.0F;\n    return std::max(-100.0F, 20.0F * std::log10(linear));\n}\n\nMeterReading AudioMeter::analyze(std::span<const float> interleaved,\n                                 std::size_t channels) noexcept {\n    MeterReading reading;\n    if (interleaved.empty() || channels == 0) return reading;\n\n    long double sumSquares = 0.0L;\n    float peak = 0.0F;\n    std::size_t validSamples = 0;\n    for (const float sample : interleaved) {\n        if (!std::isfinite(sample)) continue;\n        const float magnitude = std::abs(sample);\n        peak = std::max(peak, magnitude);\n        sumSquares += static_cast<long double>(sample) * static_cast<long double>(sample);\n        ++validSamples;\n    }\n\n    if (validSamples == 0) return reading;\n    reading.peak = peak;\n    const long double meanSquare = sumSquares / static_cast<long double>(validSamples);\n    reading.rms = static_cast<float>(std::sqrt(meanSquare));\n    reading.peakDbfs = toDbfs(reading.peak);\n    reading.rmsDbfs = toDbfs(reading.rms);\n    (void)channels;\n    return reading;\n}\n\n} // namespace ccos::audio\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/color/ColorPipeline.hpp",
        r'''#pragma once\n\n#include <QString>\n\nnamespace ccos::color {\n\nenum class ColorSpace {\n    SRgb,\n    Rec709,\n    DisplayP3,\n    Rec2020,\n    PQ,\n    HLG\n};\n\nstruct ColorSettings {\n    ColorSpace input = ColorSpace::Rec709;\n    ColorSpace output = ColorSpace::Rec709;\n    int bitDepth = 8;\n    float exposure = 0.0F;\n    float contrast = 1.0F;\n    float saturation = 1.0F;\n    bool hdr = false;\n\n    [[nodiscard]] bool validate(QString* error = nullptr) const;\n};\n\n[[nodiscard]] QString colorSpaceName(ColorSpace space);\n\n} // namespace ccos::color\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/color/ColorPipeline.cpp",
        r'''#include "color/ColorPipeline.hpp"\n\n#include <cmath>\n\nnamespace ccos::color {\n\nQString colorSpaceName(ColorSpace space) {\n    switch (space) {\n        case ColorSpace::SRgb: return QStringLiteral("sRGB");\n        case ColorSpace::Rec709: return QStringLiteral("Rec.709");\n        case ColorSpace::DisplayP3: return QStringLiteral("Display P3");\n        case ColorSpace::Rec2020: return QStringLiteral("Rec.2020");\n        case ColorSpace::PQ: return QStringLiteral("PQ");\n        case ColorSpace::HLG: return QStringLiteral("HLG");\n    }\n    return QStringLiteral("Unknown");\n}\n\nbool ColorSettings::validate(QString* error) const {\n    if (bitDepth != 8 && bitDepth != 10 && bitDepth != 12) {\n        if (error) *error = QStringLiteral("Color bit depth must be 8, 10 or 12 bits");\n        return false;\n    }\n    if (!std::isfinite(exposure) || !std::isfinite(contrast) || !std::isfinite(saturation)) {\n        if (error) *error = QStringLiteral("Color parameters must be finite");\n        return false;\n    }\n    if (contrast < 0.0F || saturation < 0.0F) {\n        if (error) *error = QStringLiteral("Contrast and saturation cannot be negative");\n        return false;\n    }\n    if ((output == ColorSpace::PQ || output == ColorSpace::HLG) && !hdr) {\n        if (error) *error = QStringLiteral("PQ/HLG output requires HDR mode");\n        return false;\n    }\n    return true;\n}\n\n} // namespace ccos::color\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/render/RenderQueueStore.hpp",
        r'''#pragma once\n\n#include <QVector>\n#include <QString>\n\nnamespace ccos::render {\n\nstruct PersistedRenderJob {\n    QString id;\n    QString output;\n    QString preset;\n    QString state = QStringLiteral("queued");\n    double progress = 0.0;\n    QString error;\n};\n\nclass RenderQueueStore final {\npublic:\n    [[nodiscard]] static bool save(const QString& path, const QVector<PersistedRenderJob>& jobs,\n                                   QString* error = nullptr);\n    [[nodiscard]] static bool load(const QString& path, QVector<PersistedRenderJob>* jobs,\n                                   QString* error = nullptr);\n};\n\n} // namespace ccos::render\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/render/RenderQueueStore.cpp",
        r'''#include "render/RenderQueueStore.hpp"\n\n#include <QJsonArray>\n#include <QJsonDocument>\n#include <QJsonObject>\n#include <QSaveFile>\n#include <QFile>\n\nnamespace ccos::render {\n\nbool RenderQueueStore::save(const QString& path, const QVector<PersistedRenderJob>& jobs, QString* error) {\n    QJsonArray array;\n    for (const auto& job : jobs) {\n        array.append(QJsonObject{\n            {QStringLiteral("id"), job.id},\n            {QStringLiteral("output"), job.output},\n            {QStringLiteral("preset"), job.preset},\n            {QStringLiteral("state"), job.state},\n            {QStringLiteral("progress"), job.progress},\n            {QStringLiteral("error"), job.error}\n        });\n    }\n\n    QSaveFile file(path);\n    if (!file.open(QIODevice::WriteOnly)) {\n        if (error) *error = file.errorString();\n        return false;\n    }\n    const QByteArray payload = QJsonDocument(QJsonObject{{QStringLiteral("version"), 1}, {QStringLiteral("jobs"), array}})\n        .toJson(QJsonDocument::Indented);\n    if (file.write(payload) != payload.size() || !file.commit()) {\n        if (error) *error = file.errorString();\n        return false;\n    }\n    return true;\n}\n\nbool RenderQueueStore::load(const QString& path, QVector<PersistedRenderJob>* jobs, QString* error) {\n    if (!jobs) {\n        if (error) *error = QStringLiteral("jobs output is required");\n        return false;\n    }\n    QFile file(path);\n    if (!file.open(QIODevice::ReadOnly)) {\n        if (error) *error = file.errorString();\n        return false;\n    }\n    QJsonParseError parseError{};\n    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);\n    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {\n        if (error) *error = parseError.errorString();\n        return false;\n    }\n    const QJsonValue version = document.object().value(QStringLiteral("version"));\n    if (!version.isDouble() || version.toInt() != 1) {\n        if (error) *error = QStringLiteral("Unsupported render queue schema version");\n        return false;\n    }\n    QVector<PersistedRenderJob> parsed;\n    const QJsonArray array = document.object().value(QStringLiteral("jobs")).toArray();\n    parsed.reserve(array.size());\n    for (const QJsonValue& value : array) {\n        const QJsonObject object = value.toObject();\n        PersistedRenderJob job;\n        job.id = object.value(QStringLiteral("id")).toString();\n        job.output = object.value(QStringLiteral("output")).toString();\n        job.preset = object.value(QStringLiteral("preset")).toString();\n        job.state = object.value(QStringLiteral("state")).toString(QStringLiteral("queued"));\n        job.progress = object.value(QStringLiteral("progress")).toDouble(0.0);\n        job.error = object.value(QStringLiteral("error")).toString();\n        if (job.id.isEmpty() || job.output.isEmpty() || job.progress < 0.0 || job.progress > 1.0) {\n            if (error) *error = QStringLiteral("Invalid render queue job entry");\n            return false;\n        }\n        parsed.append(std::move(job));\n    }\n    *jobs = std::move(parsed);\n    return true;\n}\n\n} // namespace ccos::render\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/plugins/PluginPolicy.hpp",
        r'''#pragma once\n\n#include <QString>\n#include <QSet>\n\nnamespace ccos::plugins {\n\nenum class PluginCapability {\n    ReadProject,\n    WriteProject,\n    ReadMedia,\n    WriteMedia,\n    Network,\n    SpawnProcess,\n    AccessDevice,\n    UiExtension\n};\n\nclass PluginPolicy final {\npublic:\n    [[nodiscard]] static QString capabilityName(PluginCapability capability);\n    [[nodiscard]] static bool isHighRisk(PluginCapability capability) noexcept;\n    [[nodiscard]] static bool validateRequested(const QSet<PluginCapability>& requested,\n                                                const QSet<PluginCapability>& granted,\n                                                QString* error = nullptr);\n};\n\n} // namespace ccos::plugins\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/plugins/PluginPolicy.cpp",
        r'''#include "plugins/PluginPolicy.hpp"\n\nnamespace ccos::plugins {\n\nQString PluginPolicy::capabilityName(PluginCapability capability) {\n    switch (capability) {\n        case PluginCapability::ReadProject: return QStringLiteral("read_project");\n        case PluginCapability::WriteProject: return QStringLiteral("write_project");\n        case PluginCapability::ReadMedia: return QStringLiteral("read_media");\n        case PluginCapability::WriteMedia: return QStringLiteral("write_media");\n        case PluginCapability::Network: return QStringLiteral("network");\n        case PluginCapability::SpawnProcess: return QStringLiteral("spawn_process");\n        case PluginCapability::AccessDevice: return QStringLiteral("access_device");\n        case PluginCapability::UiExtension: return QStringLiteral("ui_extension");\n    }\n    return QStringLiteral("unknown");\n}\n\nbool PluginPolicy::isHighRisk(PluginCapability capability) noexcept {\n    return capability == PluginCapability::WriteProject ||\n           capability == PluginCapability::WriteMedia ||\n           capability == PluginCapability::Network ||\n           capability == PluginCapability::SpawnProcess ||\n           capability == PluginCapability::AccessDevice;\n}\n\nbool PluginPolicy::validateRequested(const QSet<PluginCapability>& requested,\n                                     const QSet<PluginCapability>& granted,\n                                     QString* error) {\n    for (const PluginCapability capability : requested) {\n        if (granted.contains(capability)) continue;\n        if (error) *error = QStringLiteral("Plugin capability not granted: %1").arg(capabilityName(capability));\n        return false;\n    }\n    return true;\n}\n\n} // namespace ccos::plugins\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/ai/AICommandPlan.hpp",
        r'''#pragma once\n\n#include <QJsonObject>\n#include <QVector>\n#include <QString>\n\nnamespace ccos::ai {\n\nstruct AICommand {\n    QString operation;\n    QString target;\n    QJsonObject arguments;\n    bool destructive = false;\n};\n\nstruct AICommandPlan {\n    QString summary;\n    QVector<AICommand> commands;\n    bool requiresConfirmation = false;\n\n    [[nodiscard]] bool validate(QString* error = nullptr) const;\n    [[nodiscard]] QJsonObject preview() const;\n};\n\n} // namespace ccos::ai\n'''.replace('\\n', '\\n'),
    )
    write_if_missing(
        "src/ai/AICommandPlan.cpp",
        r'''#include "ai/AICommandPlan.hpp"\n\n#include <QJsonArray>\n#include <QSet>\n\nnamespace ccos::ai {\n\nnamespace {\nconst QSet<QString> kAllowedOperations = {\n    QStringLiteral("set_project_name"),\n    QStringLiteral("timeline_slip"),\n    QStringLiteral("timeline_ripple_delete"),\n    QStringLiteral("export")\n};\n}\n\nbool AICommandPlan::validate(QString* error) const {\n    if (summary.trimmed().isEmpty()) {\n        if (error) *error = QStringLiteral("AI command plan summary is required");\n        return false;\n    }\n    for (const AICommand& command : commands) {\n        if (!kAllowedOperations.contains(command.operation)) {\n            if (error) *error = QStringLiteral("Unsupported AI operation: %1").arg(command.operation);\n            return false;\n        }\n        if (command.target.size() > 512) {\n            if (error) *error = QStringLiteral("AI command target is too long");\n            return false;\n        }\n        if (command.arguments.size() > 64) {\n            if (error) *error = QStringLiteral("AI command argument object is too large");\n            return false;\n        }\n    }\n    return true;\n}\n\nQJsonObject AICommandPlan::preview() const {\n    QJsonArray commandsJson;\n    for (const AICommand& command : commands) {\n        commandsJson.append(QJsonObject{\n            {QStringLiteral("operation"), command.operation},\n            {QStringLiteral("target"), command.target},\n            {QStringLiteral("arguments"), command.arguments},\n            {QStringLiteral("destructive"), command.destructive}\n        });\n    }\n    return QJsonObject{\n        {QStringLiteral("summary"), summary},\n        {QStringLiteral("requiresConfirmation"), requiresConfirmation},\n        {QStringLiteral("commands"), commandsJson}\n    };\n}\n\n} // namespace ccos::ai\n'''.replace('\\n', '\\n'),
    )

    # Wire the new primitives into the core target.
    replace_once(
        "CMakeLists.txt",
        "    src/audio/AudioMixer.cpp\n",
        "    src/audio/AudioMixer.cpp\n    src/audio/AudioMeter.cpp\n",
    )
    replace_once(
        "CMakeLists.txt",
        "    src/ai/AdvancedAI.cpp\n",
        "    src/ai/AdvancedAI.cpp\n    src/ai/AICommandPlan.cpp\n",
    )
    replace_once(
        "CMakeLists.txt",
        "    src/plugins/PluginManager.cpp\n",
        "    src/plugins/PluginManager.cpp\n    src/plugins/PluginPolicy.cpp\n",
    )
    replace_once(
        "CMakeLists.txt",
        "    src/render/RenderExecutor.cpp\n",
        "    src/render/RenderExecutor.cpp\n    src/render/RenderQueueStore.cpp\n    src/color/ColorPipeline.cpp\n",
    )

    # Add deterministic tests for the new primitives.
    replace_once(
        "CMakeLists.txt",
        "        tests/unit/core/MitFoundationSmokeTest.cpp\n",
        "        tests/unit/core/MitFoundationSmokeTest.cpp\n        tests/unit/audio/AudioMeterTest.cpp\n        tests/unit/color/ColorPipelineTest.cpp\n        tests/unit/render/RenderQueueStoreTest.cpp\n        tests/unit/plugins/PluginPolicyTest.cpp\n        tests/unit/ai/AICommandPlanTest.cpp\n",
    )
    write_if_missing(
        "tests/unit/audio/AudioMeterTest.cpp",
        '''#include "audio/AudioMeter.hpp"\n#include <gtest/gtest.h>\n\nTEST(AudioMeterTest, ComputesPeakAndRms) {\n    const float samples[] = {1.0F, -1.0F, 0.0F, 0.0F};\n    const auto reading = ccos::audio::AudioMeter::analyze(samples, 2);\n    EXPECT_FLOAT_EQ(reading.peak, 1.0F);\n    EXPECT_NEAR(reading.rms, 0.7071067F, 1e-5F);\n    EXPECT_NEAR(reading.peakDbfs, 0.0F, 1e-5F);\n}\n''',
    )
    write_if_missing(
        "tests/unit/color/ColorPipelineTest.cpp",
        '''#include "color/ColorPipeline.hpp"\n#include <gtest/gtest.h>\n\nTEST(ColorPipelineTest, RejectsUnsupportedBitDepth) {\n    ccos::color::ColorSettings settings;\n    settings.bitDepth = 9;\n    EXPECT_FALSE(settings.validate());\n}\n\nTEST(ColorPipelineTest, RequiresHdrForPq) {\n    ccos::color::ColorSettings settings;\n    settings.output = ccos::color::ColorSpace::PQ;\n    settings.hdr = false;\n    EXPECT_FALSE(settings.validate());\n}\n''',
    )
    write_if_missing(
        "tests/unit/render/RenderQueueStoreTest.cpp",
        '''#include "render/RenderQueueStore.hpp"\n#include <gtest/gtest.h>\n#include <QTemporaryDir>\n\nTEST(RenderQueueStoreTest, RoundTripsAtomically) {\n    QTemporaryDir dir;\n    ASSERT_TRUE(dir.isValid());\n    const QString path = dir.filePath("queue.json");\n    QVector<ccos::render::PersistedRenderJob> jobs{{QStringLiteral("job-1"), QStringLiteral("out.mp4"), QStringLiteral("h264"), QStringLiteral("queued"), 0.25, {}}};\n    QString error;\n    ASSERT_TRUE(ccos::render::RenderQueueStore::save(path, jobs, &error)) << error.toStdString();\n\n    QVector<ccos::render::PersistedRenderJob> loaded;\n    ASSERT_TRUE(ccos::render::RenderQueueStore::load(path, &loaded, &error)) << error.toStdString();\n    ASSERT_EQ(loaded.size(), 1);\n    EXPECT_EQ(loaded.first().id, QStringLiteral("job-1"));\n    EXPECT_DOUBLE_EQ(loaded.first().progress, 0.25);\n}\n''',
    )
    write_if_missing(
        "tests/unit/plugins/PluginPolicyTest.cpp",
        '''#include "plugins/PluginPolicy.hpp"\n#include <gtest/gtest.h>\n\nTEST(PluginPolicyTest, MissingCapabilityIsRejected) {\n    using ccos::plugins::PluginCapability;\n    EXPECT_FALSE(ccos::plugins::PluginPolicy::validateRequested(\n        {PluginCapability::Network}, {}, nullptr));\n}\n\nTEST(PluginPolicyTest, HighRiskCapabilitiesAreMarked) {\n    using ccos::plugins::PluginCapability;\n    EXPECT_TRUE(ccos::plugins::PluginPolicy::isHighRisk(PluginCapability::SpawnProcess));\n    EXPECT_FALSE(ccos::plugins::PluginPolicy::isHighRisk(PluginCapability::ReadMedia));\n}\n''',
    )
    write_if_missing(
        "tests/unit/ai/AICommandPlanTest.cpp",
        '''#include "ai/AICommandPlan.hpp"\n#include <gtest/gtest.h>\n\nTEST(AICommandPlanTest, RejectsUnknownOperation) {\n    ccos::ai::AICommandPlan plan;\n    plan.summary = QStringLiteral("Do something");\n    plan.commands.append({QStringLiteral("delete_everything"), {}, {}, true});\n    EXPECT_FALSE(plan.validate());\n}\n\nTEST(AICommandPlanTest, ProducesPreview) {\n    ccos::ai::AICommandPlan plan;\n    plan.summary = QStringLiteral("Rename project");\n    plan.commands.append({QStringLiteral("set_project_name"), QStringLiteral("project"), {}, false});\n    const QJsonObject preview = plan.preview();\n    EXPECT_EQ(preview.value(QStringLiteral("summary")).toString(), QStringLiteral("Rename project"));\n    EXPECT_EQ(preview.value(QStringLiteral("commands")).toArray().size(), 1);\n}\n''',
    )

    # Production gates and explicit scope.
    write_if_missing(
        "docs/PRODUCTION_GATES.md",
        '''# CCOS Production Gates\n\nThis document defines what must be true before a CCOS 1.0 release can be called production-ready.\n\n## Gate A — Build integrity\n\n- Linux, Windows and macOS CI builds are green.\n- WebAssembly configure/build is green.\n- No newly introduced compiler warnings in CCOS-owned code.\n- Release artifacts are reproducible from a tagged commit.\n\n## Gate B — Editing\n\n- Interactive timeline manipulation is frame-accurate.\n- Preview is timeline-composed, not source-only.\n- Undo/redo covers every destructive editor mutation.\n- Save/load/recovery survives interrupted writes.\n\n## Gate C — Media\n\n- Missing-media detection/relink works.\n- Proxy lifecycle is observable and recoverable.\n- Decode/cache paths are bounded and cancellable.\n\n## Gate D — Render/audio/color\n\n- Render queue survives restart.\n- Hardware acceleration falls back safely.\n- Audio/video sync is deterministic.\n- 10/12-bit and HDR paths have explicit validation.\n\n## Gate E — Extensibility/AI\n\n- Plugin capabilities are explicit and denied by default.\n- AI operations are schema-validated, previewable and undoable.\n- Network/process/file capabilities are policy-gated.\n\n## Gate F — Security/legal\n\n- Sanitizers, CodeQL and dependency scanning pass.\n- Fuzz tests cover project/schema/provider parsers.\n- SPDX/SBOM/license reports are generated for releases.\n- Third-party media keeps item-specific rights metadata.\n\n## Gate G — Distribution\n\n- Native installers and runtime dependencies are verified.\n- Windows/macOS signing and notarization are configured for release builds.\n- Crash reports contain actionable version/build information without secrets.\n''',
    )
    write_if_missing(
        "docs/NLE_ROADMAP.md",
        '''# CCOS NLE Roadmap\n\n## Completed foundation\n\nProject persistence, media import/probing, timeline domain operations, undo/redo, recovery, FFmpeg export, proxy/thumbnail/waveform primitives, hardware encoder probing, API adapters, local AI provider boundaries, automation safety, CMake presets and cross-platform CI.\n\n## Next implementation layers\n\n1. Timeline canvas and direct manipulation.\n2. Timeline-composed playback/decode pipeline.\n3. GPU compositor and render graph execution.\n4. Professional audio routing, meters and effects.\n5. Effects/transitions inspector and keyframes.\n6. Color management, scopes, LUTs and HDR.\n7. Persistent render queue and diagnostics UI.\n8. Plugin SDK, permissions and process isolation.\n9. AI agent transactions, preview/apply/reject and budgets.\n10. WebAssembly parity for the supported browser feature set.\n11. Golden-media integration tests, fuzzing and performance benchmarks.\n12. Packaging, signing, SBOM and release automation.\n\nA feature is considered complete only when it has implementation, error handling, automated coverage and release documentation.\n''',
    )

    # Keep the implementation status synchronized enough that future agents do not
    # confuse the new primitives with a completed end-user feature.
    status = ROOT / "docs/IMPLEMENTATION_STATUS.md"
    text = status.read_text(encoding="utf-8")
    if "AudioMeter primitive" not in text:
        text += "\n## Newly hardened foundation\n\n- AudioMeter peak/RMS/dBFS analysis primitive.\n- ColorSettings validation boundary for bit-depth/HDR workflows.\n- Persistent render queue JSON store with atomic writes and schema validation.\n- Plugin capability policy primitive with high-risk classification.\n- AI command plan validation and deterministic preview serialization.\n\nThese primitives are not counted as a finished end-user workspace; the remaining NLE/UI work below still requires real integration and automated product-level verification.\n"
        status.write_text(text, encoding="utf-8")

    print("CCOS one-shot hardening applied successfully")


if __name__ == "__main__":
    main()
