#include "core/CrashDiagnostics.hpp"

#include <gtest/gtest.h>

TEST(CrashDiagnosticsTest, ReturnsUsableDiagnosticText) {
    const QString trace = ccos::core::CrashDiagnostics::captureCurrentStackTrace();
    EXPECT_FALSE(trace.trimmed().isEmpty());
#ifdef CCOS_HAS_CPPTRACE
    EXPECT_FALSE(trace.contains(QStringLiteral("backend disabled")));
#else
    EXPECT_NE(trace.indexOf(QStringLiteral("backend disabled")), -1);
#endif
}
