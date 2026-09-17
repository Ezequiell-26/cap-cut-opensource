#include "ai/AICommandPlan.hpp"

#include <gtest/gtest.h>

TEST(AICommandPlanTest, RejectsUnknownOperation) {
    ccos::ai::AICommandPlan plan;
    plan.summary = QStringLiteral("Run unsafe operation");
    plan.commands.append({QStringLiteral("delete_everything"), {}, {}, true});
    plan.requiresConfirmation = true;
    EXPECT_FALSE(plan.validate());
}

TEST(AICommandPlanTest, RequiresConfirmationForDestructiveCommands) {
    ccos::ai::AICommandPlan plan;
    plan.summary = QStringLiteral("Delete selected clip");
    plan.commands.append({QStringLiteral("timeline_ripple_delete"), QStringLiteral("clip-1"), {}, true});
    EXPECT_FALSE(plan.validate());
    plan.requiresConfirmation = true;
    EXPECT_TRUE(plan.validate());
}

TEST(AICommandPlanTest, ExposesDeterministicPreview) {
    ccos::ai::AICommandPlan plan;
    plan.summary = QStringLiteral("Rename project");
    plan.commands.append({QStringLiteral("set_project_name"), QStringLiteral("project"), {}, false});
    const QJsonObject preview = plan.preview();
    EXPECT_EQ(preview.value(QStringLiteral("summary")).toString(), QStringLiteral("Rename project"));
    EXPECT_EQ(preview.value(QStringLiteral("commands")).toArray().size(), 1);
}
