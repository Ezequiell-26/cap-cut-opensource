#pragma once

#include <QString>

namespace ccos::core {

class CrashDiagnostics final {
public:
    [[nodiscard]] static QString captureCurrentStackTrace();
};

} // namespace ccos::core
