#include "text/SubtitleParser.hpp"
#include <QFile>
#include <QTextStream>

namespace ccos::text {
namespace {
ccos::core::Time parseTime(const QString& value) {
    const auto parts = value.trimmed().split(':');
    if (parts.size() != 3) return {};
    QString fraction = parts.at(2);
    fraction.replace(',', '.');
    const double seconds = fraction.toDouble();
    const qint64 hours = parts.at(0).toLongLong();
    const qint64 minutes = parts.at(1).toLongLong();
    return ccos::core::Time::fromSeconds(static_cast<double>(hours * 3600 + minutes * 60) + seconds);
}
QString formatTime(ccos::core::Time t) {
    const qint64 total = static_cast<qint64>(t.seconds() * 1000.0 + 0.5);
    const qint64 h = total / 3600000, m = (total / 60000) % 60, s = (total / 1000) % 60, ms = total % 1000;
    return QStringLiteral("%1:%2:%3,%4").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')).arg(ms,3,10,QChar('0'));
}
}

bool SubtitleParser::parseSrt(const QString& path, QVector<SubtitleCue>& cues, QString* error) {
    cues.clear(); QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { if (error) *error = f.errorString(); return false; }
    QTextStream in(&f); QString line;
    while (!in.atEnd()) {
        line = in.readLine().trimmed(); if (line.isEmpty()) continue;
        if (line.at(0).isDigit() && !line.contains(QStringLiteral("-->"))) line = in.readLine().trimmed();
        const auto range = line.split(QStringLiteral("-->")); if (range.size() != 2) { if (error) *error = QStringLiteral("Invalid SRT timestamp range"); return false; }
        SubtitleCue cue; cue.start = parseTime(range.at(0)); cue.end = parseTime(range.at(1));
        QStringList text; while (!in.atEnd()) { const auto t = in.readLine(); if (t.trimmed().isEmpty()) break; text << t; }
        cue.text = text.join('\n'); cues.append(std::move(cue));
    }
    return true;
}

bool SubtitleParser::parseVtt(const QString& path, QVector<SubtitleCue>& cues, QString* error) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { if (error) *error = f.errorString(); return false; }
    QTextStream in(&f); cues.clear(); if (!in.atEnd()) in.readLine();
    while (!in.atEnd()) {
        const QString line = in.readLine(); if (line.trimmed().isEmpty() || !line.contains(QStringLiteral("-->"))) continue;
        const auto range = line.split(QStringLiteral("-->")); if (range.size() != 2) continue;
        SubtitleCue cue; cue.start = parseTime(range.at(0)); cue.end = parseTime(range.at(1).split(' ').first());
        QStringList text; while (!in.atEnd()) { const auto t = in.readLine(); if (t.trimmed().isEmpty()) break; text << t; }
        cue.text = text.join('\n'); cues.append(std::move(cue));
    }
    if (cues.isEmpty() && error) *error = QStringLiteral("No VTT cues found");
    return !cues.isEmpty();
}

bool SubtitleParser::writeSrt(const QString& path, const QVector<SubtitleCue>& cues, QString* error) {
    QFile f(path); if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) { if (error) *error = f.errorString(); return false; }
    QTextStream out(&f); for (qsizetype i = 0; i < cues.size(); ++i) out << (i + 1) << "\n" << formatTime(cues.at(i).start) << " --> " << formatTime(cues.at(i).end) << "\n" << cues.at(i).text << "\n\n";
    return true;
}
}
