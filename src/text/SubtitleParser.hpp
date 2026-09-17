#pragma once
#include "core/Time.hpp"
#include <QString>
#include <QVector>

namespace ccos::text {
struct SubtitleCue {
    ccos::core::Time start;
    ccos::core::Time end;
    QString text;
};

class SubtitleParser {
public:
    static bool parseSrt(const QString& path, QVector<SubtitleCue>& cues, QString* error = nullptr);
    static bool parseVtt(const QString& path, QVector<SubtitleCue>& cues, QString* error = nullptr);
    static bool writeSrt(const QString& path, const QVector<SubtitleCue>& cues, QString* error = nullptr);
};
}
