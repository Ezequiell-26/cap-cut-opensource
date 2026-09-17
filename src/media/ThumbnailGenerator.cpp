#include "media/ThumbnailGenerator.hpp"
#include "media/MediaCache.hpp"
#include "core/ProcessRunner.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace ccos::media {
namespace {

constexpr int kMinDimension = 16;
constexpr int kMaxDimension = 4096;
constexpr qint64 kMaxPositionMs = 24LL * 60LL * 60LL * 1000LL;
constexpr qint64 kMaxThumbnailBytes = 4LL * 1024LL * 1024LL;

bool samePath(const QString& left, const QString& right) {
    const QString a = QDir::cleanPath(QFileInfo(left).absoluteFilePath());
    const QString b = QDir::cleanPath(QFileInfo(right).absoluteFilePath());
#ifdef Q_OS_WIN
    return QString::compare(a, b, Qt::CaseInsensitive) == 0;
#else
    return a == b;
#endif
}

bool validDimension(int value) noexcept {
    return value >= kMinDimension && value <= kMaxDimension;
}

bool fail(QString* error, const QString& message) {
    if (error) *error = message;
    return false;
}

} // namespace

QString ThumbnailGenerator::thumbnailPath(const MediaCache& cache,
                                          const MediaAsset& asset,
                                          qint64 positionMs,
                                          int width,
                                          int height) {
    if (positionMs < 0 || positionMs > kMaxPositionMs ||
        !validDimension(width) || !validDimension(height)) return {};
    const QString variant = QStringLiteral("thumbnail_%1x%2_%3ms")
        .arg(width)
        .arg(height)
        .arg(positionMs);
    return cache.pathFor(asset.path(), variant, QStringLiteral("jpg"));
}

QStringList ThumbnailGenerator::buildArguments(const MediaAsset& asset,
                                                const QString& outputPath,
                                                qint64 positionMs,
                                                int width,
                                                int height,
                                                QString* error) {
    if (asset.path().trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Thumbnail input path is required");
        return {};
    }
    if (outputPath.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Thumbnail output path is required");
        return {};
    }
    if (positionMs < 0 || positionMs > kMaxPositionMs) {
        if (error) *error = QStringLiteral("Thumbnail position is outside supported bounds");
        return {};
    }
    if (!validDimension(width) || !validDimension(height)) {
        if (error) *error = QStringLiteral("Thumbnail dimensions must be between 16 and 4096 pixels");
        return {};
    }
    if (samePath(asset.path(), outputPath)) {
        if (error) *error = QStringLiteral("Thumbnail output path must not overwrite the source media");
        return {};
    }

    const QString filter = QStringLiteral(
        "scale=%1:%2:force_original_aspect_ratio=decrease:flags=lanczos,pad=%1:%2:(ow-iw)/2:(oh-ih)/2:color=black")
        .arg(width).arg(height);

    return {
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"), QStringLiteral("error"),
        QStringLiteral("-y"),
        QStringLiteral("-ss"), QString::number(static_cast<double>(positionMs) / 1000.0, 'f', 3),
        QStringLiteral("-i"), asset.path(),
        QStringLiteral("-frames:v"), QStringLiteral("1"),
        QStringLiteral("-an"), QStringLiteral("-sn"),
        QStringLiteral("-vf"), filter,
        QStringLiteral("-q:v"), QStringLiteral("3"),
        outputPath
    };
}

bool ThumbnailGenerator::generate(const MediaAsset& asset,
                                   const QString& outputPath,
                                   qint64 positionMs,
                                   int width,
                                   int height,
                                   const QString& executable,
                                   QString* error) {
    QString localError;
    QStringList args = buildArguments(asset, outputPath, positionMs, width, height, &localError);
    if (args.isEmpty()) return fail(error, localError);

    if (!ccos::core::ProcessRunner::validateExecutable(executable)) {
        return fail(error, QStringLiteral("FFmpeg executable not found or not executable: %1").arg(executable));
    }

    const QFileInfo outputInfo(outputPath);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        return fail(error, QStringLiteral("Unable to create thumbnail directory: %1").arg(outputInfo.absolutePath()));
    }

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.startupTimeout = std::chrono::seconds(5);
    config.timeout = std::chrono::seconds(30);
    config.maxOutputSize = 2 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::High;
    config.sanitizeEnvironment = true;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) return fail(error, result.errorMessage());

    const QFileInfo generated(outputPath);
    if (!generated.exists() || generated.size() <= 0) {
        return fail(error, QStringLiteral("FFmpeg completed without producing a thumbnail"));
    }
    if (generated.size() > kMaxThumbnailBytes) {
        QFile::remove(outputPath);
        return fail(error, QStringLiteral("Generated thumbnail exceeded the 4 MiB safety limit"));
    }
    return true;
}

bool ThumbnailGenerator::ensure(const MediaCache& cache,
                                 const MediaAsset& asset,
                                 qint64 positionMs,
                                 int width,
                                 int height,
                                 const QString& executable,
                                 QString* error) {
    const QString output = thumbnailPath(cache, asset, positionMs, width, height);
    if (output.isEmpty()) return fail(error, QStringLiteral("Invalid thumbnail cache parameters"));

    const QFileInfo cached(output);
    if (cached.exists() && cached.size() > 0 && cached.size() <= kMaxThumbnailBytes) return true;
    if (cached.exists()) QFile::remove(output);

    return generate(asset, output, positionMs, width, height, executable, error);
}

} // namespace ccos::media
