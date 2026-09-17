#include "core/AITooling.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {

class TestTool final : public ccos::core::AITool {
public:
    TestTool() = default;

    QString name() const override { return QStringLiteral("test.mutate"); }
    QString description() const override { return QStringLiteral("Test mutation tool"); }
    std::vector<ccos::core::ToolParameterSchema> inputSchema() const override {
        return {{QStringLiteral("value"), QStringLiteral("string"), QStringLiteral("Value"), true,
                 {}, {}, 1, 32}};
    }
    std::vector<ccos::core::ToolParameterSchema> outputSchema() const override { return {}; }
    ccos::core::ToolRiskLevel riskLevel() const override {
        return ccos::core::ToolRiskLevel::SENSITIVE_MUTATION;
    }
    QStringList requiredPermissions() const override { return {QStringLiteral("timeline.write")}; }
    bool requiresConfirmation() const override { return true; }

    ccos::core::ToolResult execute(const QVariantMap&, const QVariantMap&) override {
        ++executionCount;
        return ccos::core::ToolResult::ok(QStringLiteral("executed"), name(), QStringLiteral("call-1"));
    }

    int executionCount = 0;
};

} // namespace

TEST(AIToolingSafetyTest, RequiresPermissionBeforeExecution) {
    ccos::core::AIToolRegistry registry;
    auto tool = std::make_shared<TestTool>();
    registry.registerTool(tool);

    const QVariantMap params{{QStringLiteral("value"), QStringLiteral("x")}};
    const auto result = registry.executeTool(QStringLiteral("test.mutate"), params);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.errorMessage.indexOf(QStringLiteral("timeline.write")), -1);
    EXPECT_EQ(tool->executionCount, 0);
}

TEST(AIToolingSafetyTest, RequiresExplicitConfirmationBeforeExecution) {
    ccos::core::AIToolRegistry registry;
    auto tool = std::make_shared<TestTool>();
    registry.registerTool(tool);

    const QVariantMap params{{QStringLiteral("value"), QStringLiteral("x")}};
    const QVariantMap context{{QStringLiteral("permissions"), QStringList{QStringLiteral("timeline.write")}}};

    const auto blocked = registry.executeTool(QStringLiteral("test.mutate"), params, context, false);
    EXPECT_FALSE(blocked.success);
    EXPECT_NE(blocked.errorMessage.indexOf(QStringLiteral("confirmation")), -1);
    EXPECT_EQ(tool->executionCount, 0);

    const auto allowed = registry.executeTool(QStringLiteral("test.mutate"), params, context, true);
    EXPECT_TRUE(allowed.success);
    EXPECT_EQ(tool->executionCount, 1);
}

TEST(AIToolingSafetyTest, DisabledToolNeverExecutes) {
    ccos::core::AIToolRegistry registry;
    auto tool = std::make_shared<TestTool>();
    registry.registerTool(tool);
    registry.setToolEnabled(QStringLiteral("test.mutate"), false);

    const QVariantMap params{{QStringLiteral("value"), QStringLiteral("x")}};
    const QVariantMap context{{QStringLiteral("permissions"), QStringList{QStringLiteral("timeline.write")}}};
    const auto result = registry.executeTool(QStringLiteral("test.mutate"), params, context, true);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.errorMessage.indexOf(QStringLiteral("disabled")), -1);
    EXPECT_EQ(tool->executionCount, 0);
}

TEST(AIToolingSafetyTest, RejectsInvalidInputBeforePermissionChecks) {
    ccos::core::AIToolRegistry registry;
    auto tool = std::make_shared<TestTool>();
    registry.registerTool(tool);

    const QVariantMap invalidParams{{QStringLiteral("value"), QString()}};
    const QVariantMap context{{QStringLiteral("permissions"), QStringList{QStringLiteral("timeline.write")}}};
    const auto result = registry.executeTool(QStringLiteral("test.mutate"), invalidParams, context, true);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.errorMessage.indexOf(QStringLiteral("too short")), -1);
    EXPECT_EQ(tool->executionCount, 0);
}
