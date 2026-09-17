#include "core/CrashDiagnostics.hpp"

#ifdef CCOS_HAS_CPPTRACE
#include <cpptrace/cpptrace.hpp>
#endif

namespace ccos::core {

QString CrashDiagnostics::captureCurrentStackTrace() {
#ifdef CCOS_HAS_CPPTRACE
    const auto trace = cpptrace::generate_trace();
    return QString::fromStdString(trace.to_string());
#else
    return QStringLiteral("Stack trace backend disabled; configure with -DCCOS_ENABLE_MIT_DIAGNOSTICS=ON.");
#endif
}

} // namespace ccos::core
