#include "render/TextComposer.hpp"
#include <QColor>
#include <algorithm>

namespace ccos::render {
namespace {
QString escapeText(QString text) {
    text.replace('\\', QStringLiteral("\\\\"));
    text.replace(QChar('\''), QStringLiteral("\\'"));
    text.replace(':', QStringLiteral("\\:"));
    text.replace(',', QStringLiteral("\\,"));
    text.replace('%', QStringLiteral("\\%"));
    text.replace('\n', QStringLiteral("\\n"));
    return text;
}
QString safeColor(QString color, double opacity) {
    QColor c(color);
    if (!c.isValid()) c = QColor(QStringLiteral("#FFFFFF"));
    const double alpha = std::clamp(opacity, 0.0, 1.0);
    return QStringLiteral("0x%1%2%3@%4")
        .arg(c.red(), 2, 16, QChar('0'))
        .arg(c.green(), 2, 16, QChar('0'))
        .arg(c.blue(), 2, 16, QChar('0'))
        .arg(alpha, 0, 'f', 3);
}
}

QString TextComposer::apply(const QString& inputLabel,
                            const ccos::text::TextLayer& layer,
                            const QString& outputLabel) {
    const QString text = escapeText(layer.text());
    if (text.isEmpty()) return inputLabel + outputLabel;

    const QString enable = QStringLiteral("between(t,%1,%2)")
        .arg(layer.start().seconds(), 0, 'f', 6)
        .arg((layer.start() + layer.duration()).seconds(), 0, 'f', 6);

    QString filter = QStringLiteral("%1drawtext=text='%2':fontcolor=%3:fontsize=%4:x=(w*%5)-text_w/2:y=(h*%6)-text_h/2:enable='%7'[%8]");
    filter = filter.arg(inputLabel);
    filter = filter.arg(text);
    filter = filter.arg(safeColor(layer.style().color, layer.style().opacity));
    filter = filter.arg(QString::number(layer.style().size, 'f', 2));
    filter = filter.arg(QString::number(layer.x(), 'f', 6));
    filter = filter.arg(QString::number(layer.y(), 'f', 6));
    filter = filter.arg(enable);
    filter = filter.arg(outputLabel);
    return filter;
}
}
