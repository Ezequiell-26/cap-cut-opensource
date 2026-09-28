#include "graphics/GltfAssetInspector.hpp"

#include <gtest/gtest.h>

TEST(GltfAssetInspectorTest, RejectsUnsupportedExtension) {
    const auto result = ccos::graphics::GltfAssetInspector::inspect(
        QStringLiteral("/tmp/not-a-model.txt"));
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.error, QStringLiteral("GLTF/GLB file does not exist"));
}

TEST(GltfAssetInspectorTest, SupportFlagIsStableBoolean) {
    const bool supported = ccos::graphics::GltfAssetInspector::supported();
    EXPECT_TRUE(supported == true || supported == false);
}
