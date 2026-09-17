#pragma once
#include "text/SubtitleParser.hpp"
#include <QString>

namespace ccos::ai {
class WhisperAdapter {
public:
    static bool transcribeToSrt(const QString& mediaPath, const QString& outputSrt,
                               const QString& executable = QStringLiteral("whisper-cli"),
                               const QString& model = {}, QString* error = nullptr);
};
}
