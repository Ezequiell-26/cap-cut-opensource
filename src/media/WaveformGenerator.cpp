#include "media/WaveformGenerator.hpp"
#include "media/MediaCache.hpp"
#include "core/ProcessRunner.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace ccos::media {
namespace {
constexpr int kMinDimension = 32;
constexpr int kMaxWidth = 8192;
constexpr int kMaxHeight = 1024;
constexpr int kMinSamplesPerSecond = 10;
constexpr int kMaxSamplesPerSecond = 1000;
constexpr qint64 kMaxWaveformBytes = 8LL * 1024LL * 1024LL;

bool samePath(const QString& left, const QString& right) {
    const QString a = QDir::cleanPath(QFileInfo(left).absoluteFilePath());
    const QString b = QDir::cleanPath(QFileInfo(right).absoluteFilePath());
#ifdef Q_OS_WIN
    return QString::compare(a, b, Qt::CaseInsensitive) == 0;
#else
    return a == b;
#endif
}

bool fail(QString* error, const QString& message) {
    if (error) *error = message;
    return false;
}

bool validParameters(int width, int height, int samplesPerSecond) {
    return width >= kMinDimension && width <= kMaxWidth &&
           height >= kMinDimension && height <= kMaxHeight &&
           samplesPerSecond >= kMinSamplesPerSecond && samplesPerSecond <= kMaxSamplesPerSecond;
}
}

QString WaveformGenerator::waveformPath(const MediaCache& cache,
                                         const MediaAsset& asset,
                                         int width,
                                         int height,
                                         int samplesPerSecond) {
    if (!validParameters(width, height, samplesPerSecond)) return {};
    const QString variant = QStringLiteral("waveform_%1x%2_%3sps")
        .arg(width).arg(height).arg(samplesPerSecond);
    return cache.pathFor(asset.path(), variant, QStringLiteral("png"));
}

QStringList WaveformGenerator::buildArguments(const MediaAsset& asset,
                                               const QString& outputPath,
                                               int width,
                                               int height,
                                               int samplesPerSecond,
                                               QString* error) {
    if (asset.path().trimmed().isEmpty()) return fail(error, QStringLiteral("Waveform input path is required")), QStringList{};
    if (outputPath.trimmed().isEmpty()) return fail(error, QStringLiteral("Waveform output path is required")), QStringList{};
    if (!validParameters(width, height, samplesPerSecond)) {
        if (error) *error = QStringLiteral("Waveform dimensions or sample density are outside supported bounds");
        return {};
    }
    if (samePath(asset.path(), outputPath)) return fail(error, QStringLiteral("Waveform output path must not overwrite the source media")), QStringList{};

    const QString filter = QStringLiteral("aformat=channel_layouts=stereo,showwavespic=s=%1x%2:split_channels=0:colors=white")
        .arg(width).arg(height);

    return {
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"), QStringLiteral("error"),
        QStringLiteral("-y"),
        QStringLiteral("-i"), asset.path(),
        QStringLiteral("-filter_complex"), filter,
        QStringLiteral("-frames:v"), QStringLiteral("1"),
        QStringLiteral("-an"), QStringLiteral("-sn"),
        QStringLiteral("-compression_level"), QStringLiteral("6"),
        outputPath
    };
}

bool WaveformGenerator::generate(const MediaAsset& asset,
                                  const QString& outputPath,
                                  int width,
                                  int height,
                                  int samplesPerSecond,
                                  const QString& executable,
                                  QString* error) {
    QString localError;
    const QStringList args = buildArguments(asset, outputPath, width, height, samplesPerSecond, &localError);
    if (args.isEmpty()) return fail(error, localError);
    if (!ccos::core::ProcessRunner::validateExecutable(executable)) {
        return fail(error, QStringLiteral("FFmpeg executable not found or not executable: %1").arg(executable));
    }

    const QFileInfo outputInfo(outputPath);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        return fail(error, QStringLiteral("Unable to create waveform directory: %1").arg(outputInfo.absolutePath()));
    }

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.startupTimeout = std::chrono::seconds(5);
    config.timeout = std::chrono::seconds(45);
    config.maxOutputSize = 2 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::High;
    config.sanitizeEnvironment = true;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) return fail(error, result.errorMessage());

    const QFileInfo generated(outputPath);
    if (!generated.exists() || generated.size() <= 0) {
        return fail(error, QStringLiteral("FFmpeg completed without producing a waveform"));
    }
    if (generated.size() > kMaxWaveformBytes) {
        QFile::remove(outputPath);
        return fail(error, QStringLiteral("Generated waveform exceeded the 8 MiB safety limit"));
    }
    return true;
}

bool WaveformGenerator::ensure(const MediaCache& cache,
                                const MediaAsset& asset,
                                int width,
                                int height,
                                int samplesPerSecond,
                                const QString& executable,
                                QString* error) {
    const QString output = waveformPath(cache, asset, width, height, samplesPerSecond);
    if (output.isEmpty()) return fail(error, QStringLiteral("Invalid waveform cache parameters"));

    const QFileInfo cached(output);
    if (cached.exists() && cached.size() > 0 && cached.size() <= kMaxWaveformBytes) return true;
    if (cached.exists()) QFile::remove(output);
    return generate(asset, output, width, height, samplesPerSecond, executable, error);
}

} // namespace ccos::media
