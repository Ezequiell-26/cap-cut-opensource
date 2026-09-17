#include "api/CreativeMediaApi.hpp"

#include <gtest/gtest.h>

TEST(CreativeMediaApiTest, ExposesOpenMediaProviders) {
    const auto providers = ccos::api::CreativeMediaApi::supportedProviders();
    EXPECT_TRUE(providers.contains(QStringLiteral("openverse")));
    EXPECT_TRUE(providers.contains(QStringLiteral("wikimedia_commons")));
}

TEST(CreativeMediaApiTest, KindNamesAreStable) {
    EXPECT_EQ(ccos::api::CreativeMediaApi::kindToString(ccos::api::CreativeMediaKind::Image), QStringLiteral("image"));
    EXPECT_EQ(ccos::api::CreativeMediaApi::kindToString(ccos::api::CreativeMediaKind::Audio), QStringLiteral("audio"));
    EXPECT_EQ(ccos::api::CreativeMediaApi::kindToString(ccos::api::CreativeMediaKind::Video), QStringLiteral("video"));
}

TEST(CreativeMediaApiTest, ItemMustHaveStableIdAndDownloadUrl) {
    ccos::api::CreativeMediaItem item;
    EXPECT_FALSE(item.isUsable());
    item.id = QStringLiteral("id");
    item.downloadUrl = QStringLiteral("https://example.invalid/media");
    EXPECT_TRUE(item.isUsable());
}
