#include "ai/LlamaCppProvider.hpp"

#include <gtest/gtest.h>

TEST(LlamaCppProviderTest, ExposesStableProviderIdentity) {
    ccos::ai::LlamaCppProvider provider;
    EXPECT_EQ(provider.id(), QStringLiteral("llama-cpp"));
    EXPECT_EQ(provider.name(), QStringLiteral("llama.cpp local server"));
}

TEST(LlamaCppProviderTest, RejectsInvalidRequestBeforeNetworkCall) {
    ccos::ai::LlamaCppProvider provider;
    ccos::ai::AIRequest request;
    request.input = QStringLiteral("hello");

    const auto response = provider.execute(request);
    EXPECT_FALSE(response.ok);
    EXPECT_EQ(response.error, QStringLiteral("AI request option 'model' is required"));
}
