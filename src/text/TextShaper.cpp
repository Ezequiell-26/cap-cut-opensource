#include "text/TextShaper.hpp"

#include <QByteArray>
#include <QtGlobal>

#include <algorithm>

#ifdef CCOS_HAS_HARFBUZZ
#include <hb.h>
#endif

namespace ccos::text {

QVector<GlyphPlacement> TextShaper::shape(const QString& text,
                                          const QString& language,
                                          const QString& script,
                                          bool rightToLeft) {
    QVector<GlyphPlacement> result;
    const QString normalized = text.normalized(QString::NormalizationForm_C);
    if (normalized.isEmpty()) return result;

#ifdef CCOS_HAS_HARFBUZZ
    const QByteArray utf8 = normalized.toUtf8();
    hb_buffer_t* buffer = hb_buffer_create();
    if (!buffer) return result;

    hb_buffer_add_utf8(buffer, utf8.constData(), utf8.size(), 0, utf8.size());
    hb_buffer_set_direction(buffer, rightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    if (!script.isEmpty()) {
        const hb_script_t hbScript = hb_script_from_string(script.toUtf8().constData(), -1);
        if (hbScript != HB_SCRIPT_INVALID) hb_buffer_set_script(buffer, hbScript);
    }
    if (!language.isEmpty()) {
        hb_language_t hbLanguage = hb_language_from_string(language.toUtf8().constData(), -1);
        hb_buffer_set_language(buffer, hbLanguage);
    }
    hb_buffer_guess_segment_properties(buffer);

    hb_face_t* face = hb_face_get_empty();
    hb_font_t* font = hb_font_create(face);
    if (!font) {
        hb_buffer_destroy(buffer);
        return result;
    }
    hb_font_set_scale(font, 1024, 1024);
    hb_shape(font, buffer, nullptr, 0);

    unsigned int glyphCount = 0;
    const hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
    const hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(buffer, &glyphCount);
    result.reserve(static_cast<qsizetype>(glyphCount));

    for (unsigned int i = 0; i < glyphCount; ++i) {
        const hb_glyph_position_t position = glyphPositions[i];
        result.append(GlyphPlacement{
            static_cast<quint32>(glyphInfos[i].codepoint),
            static_cast<quint32>(glyphInfos[i].cluster),
            static_cast<double>(position.x_advance) / 1024.0,
            static_cast<double>(position.x_offset) / 1024.0,
            static_cast<double>(position.y_offset) / 1024.0
        });
    }

    hb_font_destroy(font);
    hb_buffer_destroy(buffer);
    return result;
#else
    result.reserve(normalized.size());
    for (int i = 0; i < normalized.size(); ++i) {
        const QChar character = normalized.at(i);
        if (character.isHighSurrogate() && i + 1 < normalized.size() && normalized.at(i + 1).isLowSurrogate()) {
            const uint codepoint = QChar::surrogateToUcs4(character, normalized.at(++i));
            result.append(GlyphPlacement{codepoint, static_cast<quint32>(i), 1.0, 0.0, 0.0});
        } else {
            result.append(GlyphPlacement{character.unicode(), static_cast<quint32>(i), 1.0, 0.0, 0.0});
        }
    }
    if (rightToLeft) std::reverse(result.begin(), result.end());
    Q_UNUSED(language);
    Q_UNUSED(script);
    return result;
#endif
}

} // namespace ccos::text
