#include "api/EditorApi.hpp"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>

namespace {
ccos::project::Project makeProject() {
    return ccos::project::Project(QStringLiteral("API Test Project"));
}
}

TEST(EditorApiTest, ExportPresetsAreMachineReadable) {
    const QJsonObject result = ccos::api::EditorApi::exportPresets();

    ASSERT_TRUE(result.value(QStringLiteral("ok")).toBool());
    const QJsonArray presets = result.value(QStringLiteral("presets")).toArray();
    ASSERT_EQ(presets.size(), 5);
    for (const auto& value : presets) {
        const QJsonObject preset = value.toObject();
        EXPECT_FALSE(preset.value(QStringLiteral("id")).toString().isEmpty());
        EXPECT_GT(preset.value(QStringLiteral("width")).toInt(), 0);
        EXPECT_GT(preset.value(QStringLiteral("height")).toInt(), 0);
        EXPECT_GT(preset.value(QStringLiteral("fps")).toDouble(), 0.0);
    }
}

TEST(EditorApiTest, UnknownExportPresetFailsBeforeStartingProcess) {
    auto project = makeProject();
    const QJsonObject request{
        {QStringLiteral("op"), QStringLiteral("export")},
        {QStringLiteral("output"), QStringLiteral("/tmp/output.mp4")},
        {QStringLiteral("preset"), QStringLiteral("not-a-real-preset")}
    };

    const QJsonObject result = ccos::api::EditorApi::command(project, request);
    EXPECT_FALSE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_EQ(result.value(QStringLiteral("error")).toString(), QStringLiteral("unknown export preset"));
}

TEST(EditorApiTest, ValidationReportsEmptyProjectWarningsWithoutErrors) {
    const auto project = makeProject();
    const QJsonObject result = ccos::api::EditorApi::validate(project);

    EXPECT_TRUE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_TRUE(result.value(QStringLiteral("errors")).toArray().isEmpty());
    EXPECT_FALSE(result.value(QStringLiteral("warnings")).toArray().isEmpty());
}
