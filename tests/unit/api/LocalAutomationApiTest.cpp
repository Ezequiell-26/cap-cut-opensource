#include "api/LocalAutomationApi.hpp"

#include <gtest/gtest.h>

TEST(LocalAutomationApiTest, DefaultsToLoopbackAgentPort) {
    ccos::api::LocalAutomationApi api(nullptr);
    EXPECT_EQ(api.port(), 47999);
    EXPECT_FALSE(api.isRunning());
}

TEST(LocalAutomationApiTest, CustomPortAndTokenAreAccepted) {
    ccos::api::LocalAutomationApi api(nullptr, 48001, QStringLiteral("test-token"));
    EXPECT_EQ(api.port(), 48001);
    EXPECT_FALSE(api.isRunning());
}
