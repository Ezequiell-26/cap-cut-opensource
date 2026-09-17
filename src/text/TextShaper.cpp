#include "text/TextShaper.hpp"

#include <QtGlobal>

#include <algorithm>

#ifdef CCOS_HAS_HARFBUZZ
#include <hb.h>
#include <hb-ot.h>
#endif

namespace ccos::text {
namespace {

QVector<GlyphPlacement> unicodeFallback(const QString& normalized, bool rightToLeft) {
    QVector<GlyphPlacement> result;
    result.reserve(normalized.size());
    for (int i = 0; i < normalized.size(); ++i) {
        const QChar character = normalized.at(i);
        const quint32 cluster = static_cast<quint32>(i);
        if (character.isHighSurrogate() && i + 1 < normalized.size() && normalized.at(i + 1).isLowSurrogate()) {
            const quint32 codepoint = QChar::surrogateToUcs4(character, normalized.at(++i));
            result.append(GlyphPlacement{codepoint, cluster, 1.0, 0.0, 0.0});
        } else {
            result.append(GlyphPlacement{character.unicode(), cluster, 1.0, 0.0, 0.0});
        }
    }
    if (rightToLeft) std::reverse(result.begin(), result.end());
    return result;
}

} // namespace

QVector<GlyphPlacement> TextShaper::shape(const QString& text,
                                          const QString& language,
                                          const QString& script,
                                          bool rightToLeft) {
    return shape(text, QByteArray{}, language, script, rightToLeft);
}

QVector<GlyphPlacement> TextShaper::shape(const QString& text,
                                          const QByteArray& fontData,
                                          const QString& language,
                                          const QString& script,
                                          bool rightToLeft) {
    const QString normalized = text.normalized(QString::NormalizationForm_C);
    if (normalized.isEmpty()) return {};

#ifdef CCOS_HAS_HARFBUZZ
    if (!fontData.isEmpty()) {
        const QByteArray utf8 = normalized.toUtf8();
        hb_blob_t* blob = hb_blob_create(fontData.constData(),
                                         static_cast<unsigned int>(fontData.size()),
                                         HB_MEMORY_MODE_READONLY,
                                         nullptr,
                                         nullptr);
        if (!blob) return unicodeFallback(normalized, rightToLeft);

        hb_face_t* face = hb_face_create(blob, 0);
        hb_blob_destroy(blob);
        if (!face) return unicodeFallback(normalized, rightToLeft);

        hb_font_t* font = hb_font_create(face);
        if (!font) {
            hb_face_destroy(face);
            return unicodeFallback(normalized, rightToLeft);
        }
        hb_ot_font_set_funcs(font);
        const unsigned int upem = hb_face_get_upem(face);
        const int scale = static_cast<int>(std::min<unsigned int>(upem, 1'000'000U));
        hb_font_set_scale(font, scale, scale);

        hb_buffer_t* buffer = hb_buffer_create();
        if (!buffer) {
            hb_font_destroy(font);
            hb_face_destroy(face);
            return unicodeFallback(normalized, rightToLeft);
        }

        hb_buffer_add_utf8(buffer, utf8.constData(), utf8.size(), 0, utf8.size());
        hb_buffer_set_direction(buffer, rightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
        if (!script.isEmpty()) {
            const QByteArray scriptUtf8 = script.toUtf8();
            const hb_script_t hbScript = hb_script_from_string(scriptUtf8.constData(), -1);
            hb_buffer_set_script(buffer, hbScript);
        }
        if (!language.isEmpty()) {
            const QByteArray languageUtf8 = language.toUtf8();
            hb_buffer_set_language(buffer, hb_language_from_string(languageUtf8.constData(), -1));
        }
        hb_buffer_guess_segment_properties(buffer);
        hb_shape(font, buffer, nullptr, 0);

        unsigned int glyphCount = 0;
        const hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
        const hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(buffer, &glyphCount);

        QVector<GlyphPlacement> result;
        result.reserve(static_cast<qsizetype>(glyphCount));
        for (unsigned int i = 0; i < glyphCount; ++i) {
            const hb_glyph_position_t position = glyphPositions[i];
            result.append(GlyphPlacement{
                static_cast<quint32>(glyphInfos[i].codepoint),
                static_cast<quint32>(glyphInfos[i].cluster),
                static_cast<double>(position.x_advance) / static_cast<double>(scale),
                static_cast<double>(position.x_offset) / static_cast<double>(scale),
                static_cast<double>(position.y_offset) / static_cast<double>(scale)
            });
        }

        hb_buffer_destroy(buffer);
        hb_font_destroy(font);
        hb_face_destroy(face);
        return result;
    }
#endif

    Q_UNUSED(language);
    Q_UNUSED(script);
    return unicodeFallback(normalized, rightToLeft);
}

} // namespace ccos::text
