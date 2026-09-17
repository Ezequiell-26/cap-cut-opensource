#include "color/ColorPipeline.hpp"

#include <cmath>

namespace ccos::color {

QString colorSpaceName(ColorSpace space) {
    switch (space) {
        case ColorSpace::SRgb: return QStringLiteral("sRGB");
        case ColorSpace::Rec709: return QStringLiteral("Rec.709");
        case ColorSpace::DisplayP3: return QStringLiteral("Display P3");
        case ColorSpace::Rec2020: return QStringLiteral("Rec.2020");
        case ColorSpace::PQ: return QStringLiteral("PQ");
        case ColorSpace::HLG: return QStringLiteral("HLG");
    }
    return QStringLiteral("Unknown");
}

bool ColorSettings::validate(QString* error) const {
    if (bitDepth != 8 && bitDepth != 10 && bitDepth != 12) {
        if (error) *error = QStringLiteral("Color bit depth must be 8, 10 or 12 bits");
        return false;
    }
    if (!std::isfinite(exposure) || !std::isfinite(contrast) || !std::isfinite(saturation)) {
        if (error) *error = QStringLiteral("Color parameters must be finite");
        return false;
    }
    if (contrast < 0.0F || saturation < 0.0F) {
        if (error) *error = QStringLiteral("Contrast and saturation cannot be negative");
        return false;
    }
    if ((output == ColorSpace::PQ || output == ColorSpace::HLG) && !hdr) {
        if (error) *error = QStringLiteral("PQ/HLG output requires HDR mode");
        return false;
    }
    return true;
}

} // namespace ccos::color
