#pragma once
#include <QString>
#include <QVariantMap>

namespace ccos::api {

struct ColorPalette {
    QString name;
    QVector<QString> colors; // Hex color codes
};

class ColorApi {
public:
    static ColorPalette generatePalette(const QString& baseColor, int count = 5);
    static ColorPalette complementaryPalette(const QString& baseColor);
    static ColorPalette analogousPalette(const QString& baseColor, int count = 3);
    static ColorPalette triadicPalette(const QString& baseColor);
    
    static QString lighten(const QString& hexColor, double factor);
    static QString darken(const QString& hexColor, double factor);
    static QVector<QString> gradient(const QString& startColor, const QString& endColor, int steps);
};

} // namespace ccos::api
