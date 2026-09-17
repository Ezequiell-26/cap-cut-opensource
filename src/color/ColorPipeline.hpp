#pragma once

#include <QString>

namespace ccos::color {

enum class ColorSpace {
    SRgb,
    Rec709,
    DisplayP3,
    Rec2020,
    PQ,
    HLG
};

struct ColorSettings {
    ColorSpace input = ColorSpace::Rec709;
    ColorSpace output = ColorSpace::Rec709;
    int bitDepth = 8;
    float exposure = 0.0F;
    float contrast = 1.0F;
    float saturation = 1.0F;
    bool hdr = false;

    [[nodiscard]] bool validate(QString* error = nullptr) const;
};

[[nodiscard]] QString colorSpaceName(ColorSpace space);

} // namespace ccos::color
