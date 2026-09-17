#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

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
    [[nodiscard]] static QVector<GlyphPlacement> shape(
        const QString& text,
        const QString& language = {},
        const QString& script = {},
        bool rightToLeft = false);

    // When HarfBuzz is enabled and valid font bytes are supplied, this overload
    // performs actual font-aware shaping. No font bytes means deterministic
    // Unicode fallback rather than pretending that glyph shaping occurred.
    [[nodiscard]] static QVector<GlyphPlacement> shape(
        const QString& text,
        const QByteArray& fontData,
        const QString& language = {},
        const QString& script = {},
        bool rightToLeft = false);
};

} // namespace ccos::text
