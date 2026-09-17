#include "color/ColorPipeline.hpp"

#include <gtest/gtest.h>

TEST(ColorPipelineTest, RejectsUnsupportedBitDepth) {
    ccos::color::ColorSettings settings;
    settings.bitDepth = 9;
    EXPECT_FALSE(settings.validate());
}

TEST(ColorPipelineTest, RequiresHdrForPq) {
    ccos::color::ColorSettings settings;
    settings.output = ccos::color::ColorSpace::PQ;
    settings.hdr = false;
    EXPECT_FALSE(settings.validate());
}
