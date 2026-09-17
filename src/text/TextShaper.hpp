#pragma once

#include <QChar>
#include <QVector>
#include <QString>

namespace ccos::text {

struct GlyphPlacement {
    quint32 glyphId = 0;
    quint32 cluster = 0;
    double advanceX = 0.0;
    double offsetX = 0.0;
    double offsetY = 0.0;
};

class TextShaper final {
public:
    // Shapes UTF-8 text using HarfBuzz when the optional backend is available.
    // The fallback preserves Unicode code points as glyph IDs and zeroes
    // offsets so callers can still render or inspect a deterministic run.
    [[nodiscard]] static QVector<GlyphPlacement> shape(const QString& text,
                                                       const QString& language = {},
                                                       const QString& script = {},
                                                       bool rightToLeft = false);
};

} // namespace ccos::text
