#include "effects/BuiltinEffects.hpp"
#include <cmath>

namespace ccos::effects {

QStringList BuiltinEffects::ids() {
    return {QStringLiteral("brightness"), QStringLiteral("contrast"), QStringLiteral("saturation"),
            QStringLiteral("exposure"), QStringLiteral("grayscale"), QStringLiteral("sepia"),
            QStringLiteral("blur"), QStringLiteral("sharpen"), QStringLiteral("vignette"), QStringLiteral("invert")};
}

BuiltinEffect BuiltinEffects::get(const QString& id) {
    BuiltinEffect e{id, id, {}};
    if (id == QStringLiteral("brightness")) { e.name = QStringLiteral("Brightness"); e.defaults = {{QStringLiteral("value"), 0.0}}; }
    else if (id == QStringLiteral("contrast")) { e.name = QStringLiteral("Contrast"); e.defaults = {{QStringLiteral("value"), 1.0}}; }
    else if (id == QStringLiteral("saturation")) { e.name = QStringLiteral("Saturation"); e.defaults = {{QStringLiteral("value"), 1.0}}; }
    else if (id == QStringLiteral("exposure")) { e.name = QStringLiteral("Exposure"); e.defaults = {{QStringLiteral("value"), 0.0}}; }
    else if (id == QStringLiteral("blur")) { e.name = QStringLiteral("Blur"); e.defaults = {{QStringLiteral("radius"), 4.0}}; }
    else if (id == QStringLiteral("sharpen")) { e.name = QStringLiteral("Sharpen"); e.defaults = {{QStringLiteral("amount"), 1.0}}; }
    else if (id == QStringLiteral("vignette")) { e.name = QStringLiteral("Vignette"); }
    else if (id == QStringLiteral("invert")) { e.name = QStringLiteral("Invert"); }
    else if (id == QStringLiteral("grayscale")) { e.name = QStringLiteral("Grayscale"); }
    else if (id == QStringLiteral("sepia")) { e.name = QStringLiteral("Sepia"); }
    return e;
}

QString BuiltinEffects::ffmpegFilter(const QString& id, const QVariantMap& parameters) {
    auto value = [&](const char* key, double fallback) { return parameters.value(QString::fromLatin1(key), fallback).toDouble(); };
    if (id == QStringLiteral("brightness")) return QStringLiteral("eq=brightness=%1").arg(value("value", 0.0), 0, 'f', 4);
    if (id == QStringLiteral("contrast")) return QStringLiteral("eq=contrast=%1").arg(value("value", 1.0), 0, 'f', 4);
    if (id == QStringLiteral("saturation")) return QStringLiteral("eq=saturation=%1").arg(value("value", 1.0), 0, 'f', 4);
    if (id == QStringLiteral("exposure")) return QStringLiteral("eq=gamma=%1").arg(std::pow(2.0, value("value", 0.0)), 0, 'f', 4);
    if (id == QStringLiteral("blur")) return QStringLiteral("boxblur=%1:1").arg(value("radius", 4.0), 0, 'f', 1);
    if (id == QStringLiteral("sharpen")) return QStringLiteral("unsharp=5:5:%1:5:5:0").arg(value("amount", 1.0), 0, 'f', 2);
    if (id == QStringLiteral("vignette")) return QStringLiteral("vignette=PI/4");
    if (id == QStringLiteral("invert")) return QStringLiteral("negate");
    if (id == QStringLiteral("grayscale")) return QStringLiteral("hue=s=0");
    if (id == QStringLiteral("sepia")) return QStringLiteral("colorchannelmixer=.393:.769:.189:0:.349:.686:.168:0:.272:.534:.131");
    return {};
}
}
