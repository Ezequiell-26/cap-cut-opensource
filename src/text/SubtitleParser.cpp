#include "text/SubtitleParser.hpp"
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace ccos::text {
namespace {
ccos::core::Time parseTime(const QString& s) {
    const auto parts = s.trimmed().split(':');
    if (parts.size() != 3) return {};
    const auto ms = parts.at(2).replace(',', '.').toDouble();
    const auto hours = parts.at(0).toLongLong();
    const auto minutes = parts.at(1).toLongLong();
    return ccos::core::Time::fromSeconds(static_cast<double>(hours * 3600 + minutes * 60) + ms);
}
QString formatTime(ccos::core::Time t) {
    const qint64 total = static_cast<qint64>(t.seconds() * 1000.0 + 0.5);
    const qint64 h = total / 3600000;
    const qint64 m = (total / 60000) % 60;
    const qint64 s = (total / 1000) % 60;
    const qint64 ms = total % 1000;
    return QStringLiteral("%1:%2:%3,%4").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')).arg(ms,3,10,QChar('0'));
}
}

bool SubtitleParser::parseSrt(const QString& path, QVector<SubtitleCue>& cues, QString* error) {
    cues.clear();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { if (error) *error = f.errorString(); return false; }
    QTextStream in(&f);
    QString line;
    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (line.at(0).isDigit()) line = in.readLine();
        const auto range = line.split(QStringLiteral("-->"));
        if (range.size() != 2) { if (error) *error = QStringLiteral("Invalid SRT timestamp range"); return false; }
        SubtitleCue cue;
        cue.start = parseTime(range.at(0));
        cue.end = parseTime(range.at(1));
        QStringList text;
        while (!in.atEnd()) { const auto t = in.readLine(); if (t.trimmed().isEmpty()) break; text << t; }
        cue.text = text.join('\n');
        cues.append(std::move(cue));
    }
    return true;
}

bool SubtitleParser::parseVtt(const QString& path, QVector<SubtitleCue>& cues, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { if (error) *error = f.errorString(); return false; }
    QTextStream in(&f); cues.clear(); QString line;
    if (!in.atEnd()) in.readLine();
    while (!in.atEnd()) {
        line = in.readLine();
        if (line.trimmed().isEmpty() || !line.contains(QStringLiteral("-->"))) continue;
        const auto range = line.split(QStringLiteral("-->"));
        if (range.size() != 2) continue;
        SubtitleCue cue;
        cue.start = parseTime(range.at(0));
        cue.end = parseTime(range.at(1).split(' ').first());
        QStringList text;
        while (!in.atEnd()) { const auto t = in.readLine(); if (t.trimmed().isEmpty()) break; text << t; }
        cue.text = text.join('\n');
        cues.append(std::move(cue));
    }
    if (cues.isEmpty() && error) *error = QStringLiteral("No VTT cues found");
    return !cues.isEmpty();
}

bool SubtitleParser::writeSrt(const QString& path, const QVector<SubtitleCue>& cues, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) { if (error) *error = f.errorString(); return false; }
    QTextStream out(&f);
    for (qsizetype i = 0; i < cues.size(); ++i) out << (i + 1) << "\n" << formatTime(cues.at(i).start) << " --> " << formatTime(cues.at(i).end) << "\n" << cues.at(i).text << "\n\n";
    return true;
}
}
