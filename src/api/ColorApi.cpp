#include "api/ColorApi.hpp"
#include <QColor>
#include <QtMath>

namespace ccos::api {

static QColor hexToColor(const QString& hex) {
    QString clean = hex.trimmed();
    if (clean.startsWith('#')) clean = clean.mid(1);
    if (clean.length() == 3) {
        clean = QString("%1%1%2%2%3%3").arg(clean[0]).arg(clean[1]).arg(clean[2]);
    }
    return QColor("#" + clean);
}

static QString colorToHex(const QColor& color) {
    return color.name(QColor::HexRgb).toUpper();
}

QString ColorApi::lighten(const QString& hexColor, double factor) {
    QColor color = hexToColor(hexColor);
    int h, s, l;
    color.getHsl(&h, &s, &l);
    l = qBound(0, l + static_cast<int>(l * factor), 255);
    color.setHsl(h, s, l);
    return colorToHex(color);
}

QString ColorApi::darken(const QString& hexColor, double factor) {
    QColor color = hexToColor(hexColor);
    int h, s, l;
    color.getHsl(&h, &s, &l);
    l = qBound(0, l - static_cast<int>(l * factor), 255);
    color.setHsl(h, s, l);
    return colorToHex(color);
}

QVector<QString> ColorApi::gradient(const QString& startColor, const QString& endColor, int steps) {
    QVector<QString> result;
    if (steps < 2) {
        if (steps == 1) result.append(startColor);
        return result;
    }
    
    QColor start = hexToColor(startColor);
    QColor end = hexToColor(endColor);
    
    for (int i = 0; i < steps; ++i) {
        double t = static_cast<double>(i) / (steps - 1);
        int r = static_cast<int>(start.red() + (end.red() - start.red()) * t);
        int g = static_cast<int>(start.green() + (end.green() - start.green()) * t);
        int b = static_cast<int>(start.blue() + (end.blue() - start.blue()) * t);
        result.append(QColor(r, g, b).name(QColor::HexRgb).toUpper());
    }
    
    return result;
}

ColorPalette ColorApi::generatePalette(const QString& baseColor, int count) {
    ColorPalette palette;
    palette.name = QStringLiteral("Generated");
    
    QColor base = hexToColor(baseColor);
    int h, s, l;
    base.getHsl(&h, &s, &l);
    
    for (int i = 0; i < count; ++i) {
        double t = static_cast<double>(i) / (count - 1);
        int newL = qBound(0, static_cast<int>(l * (0.3 + t * 1.4)), 255);
        QColor variant(h, s, newL);
        palette.colors.append(colorToHex(variant));
    }
    
    return palette;
}

ColorPalette ColorApi::complementaryPalette(const QString& baseColor) {
    ColorPalette palette;
    palette.name = QStringLiteral("Complementary");
    
    QColor base = hexToColor(baseColor);
    int h, s, l;
    base.getHsl(&h, &s, &l);
    
    // Base color
    palette.colors.append(colorToHex(base));
    
    // Complementary (180 degrees)
    int compH = (h + 180) % 360;
    QColor complementary(compH, s, l);
    palette.colors.append(colorToHex(complementary));
    
    // Lightened base
    palette.colors.append(lighten(baseColor, 0.3));
    
    // Darkened base
    palette.colors.append(darken(baseColor, 0.3));
    
    return palette;
}

ColorPalette ColorApi::analogousPalette(const QString& baseColor, int count) {
    ColorPalette palette;
    palette.name = QStringLiteral("Analogous");
    
    QColor base = hexToColor(baseColor);
    int h, s, l;
    base.getHsl(&h, &s, &l);
    
    int step = 30; // 30 degrees between analogous colors
    int startOffset = -((count - 1) / 2) * step;
    
    for (int i = 0; i < count; ++i) {
        int newH = (h + startOffset + i * step + 360) % 360;
        QColor variant(newH, s, l);
        palette.colors.append(colorToHex(variant));
    }
    
    return palette;
}

ColorPalette ColorApi::triadicPalette(const QString& baseColor) {
    ColorPalette palette;
    palette.name = QStringLiteral("Triadic");
    
    QColor base = hexToColor(baseColor);
    int h, s, l;
    base.getHsl(&h, &s, &l);
    
    // Base color
    palette.colors.append(colorToHex(base));
    
    // First triadic (120 degrees)
    int h1 = (h + 120) % 360;
    QColor first(h1, s, l);
    palette.colors.append(colorToHex(first));
    
    // Second triadic (240 degrees)
    int h2 = (h + 240) % 360;
    QColor second(h2, s, l);
    palette.colors.append(colorToHex(second));
    
    return palette;
}

} // namespace ccos::api
