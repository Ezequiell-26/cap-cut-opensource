#include "render/ExportSettings.hpp"

#include <cmath>

namespace ccos::render {
namespace {

bool validToken(const QString& value, int maxLength) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty() || trimmed.size() > maxLength || trimmed.startsWith(QLatin1Char('-'))) return false;
    for (const QChar c : trimmed) {
        const bool valid = (c >= QLatin1Char('a') && c <= QLatin1Char('z')) ||
                           (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) ||
                           (c >= QLatin1Char('0') && c <= QLatin1Char('9')) ||
                           c == QLatin1Char('_') || c == QLatin1Char('-') || c == QLatin1Char('.');
        if (!valid) return false;
    }
    return true;
}

bool fail(QString* error, const QString& message) {
    if (error) *error = message;
    return false;
}

} // namespace

bool ExportSettings::validate(QString* error) const {
    if (width < 16 || width > 16384) {
        return fail(error, QStringLiteral("Export width must be between 16 and 16384 pixels"));
    }
    if (height < 16 || height > 16384) {
        return fail(error, QStringLiteral("Export height must be between 16 and 16384 pixels"));
    }
    if (!std::isfinite(fps) || fps < 1.0 || fps > 240.0) {
        return fail(error, QStringLiteral("Export FPS must be finite and between 1 and 240"));
    }
    if (videoBitrateKbps < 32 || videoBitrateKbps > 1'000'000) {
        return fail(error, QStringLiteral("Video bitrate must be between 32 and 1000000 kbps"));
    }
    if (audioBitrateKbps < 8 || audioBitrateKbps > 2000) {
        return fail(error, QStringLiteral("Audio bitrate must be between 8 and 2000 kbps"));
    }
    if (!validToken(container, 16)) {
        return fail(error, QStringLiteral("Export container is invalid"));
    }
    if (!validToken(videoCodec, 64)) {
        return fail(error, QStringLiteral("Video codec is invalid"));
    }
    if (!validToken(audioCodec, 64)) {
        return fail(error, QStringLiteral("Audio codec is invalid"));
    }
    return true;
}

} // namespace ccos::render
