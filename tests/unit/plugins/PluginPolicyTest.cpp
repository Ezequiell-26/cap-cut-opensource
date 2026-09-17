#include "plugins/PluginPolicy.hpp"

#include <gtest/gtest.h>

TEST(PluginPolicyTest, DeniesUnlistedCapabilities) {
    using ccos::plugins::PluginCapability;
    QString error;
    EXPECT_FALSE(ccos::plugins::PluginPolicy::validateRequested(
        {PluginCapability::Network}, {}, &error));
    EXPECT_NE(error, QString());
}

TEST(PluginPolicyTest, ClassifiesHighRiskCapabilities) {
    using ccos::plugins::PluginCapability;
    EXPECT_TRUE(ccos::plugins::PluginPolicy::isHighRisk(PluginCapability::SpawnProcess));
    EXPECT_TRUE(ccos::plugins::PluginPolicy::isHighRisk(PluginCapability::WriteProject));
    EXPECT_FALSE(ccos::plugins::PluginPolicy::isHighRisk(PluginCapability::ReadMedia));
}
