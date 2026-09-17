#include "ai/WhisperAdapter.hpp"
#include "core/ProcessRunner.hpp"

#include <QFile>
#include <QFileInfo>

namespace ccos::ai {

bool WhisperAdapter::transcribeToSrt(const QString& mediaPath, const QString& outputSrt,
                                     const QString& executable, const QString& model, QString* error) {
    if (mediaPath.isEmpty() || outputSrt.isEmpty()) {
        if (error) *error = QStringLiteral("Media and output paths are required");
        return false;
    }

    QStringList args{
        QStringLiteral("-m"), model,
        QStringLiteral("-f"), mediaPath,
        QStringLiteral("-osrt"),
        QStringLiteral("-of"), QFileInfo(outputSrt).path() + QLatin1Char('/') + QFileInfo(outputSrt).completeBaseName()
    };
    if (model.isEmpty()) {
        args.removeFirst();
        args.removeFirst();
    }

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.timeout = std::chrono::minutes(30);
    config.startupTimeout = std::chrono::seconds(5);
    config.maxOutputSize = 32 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::High;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) {
        if (error) *error = result.errorMessage();
        return false;
    }

    const QString generated = QFileInfo(outputSrt).path() + QLatin1Char('/') +
                              QFileInfo(outputSrt).completeBaseName() + QStringLiteral(".srt");
    if (generated != outputSrt && QFileInfo::exists(generated)) {
        if (QFileInfo::exists(outputSrt)) QFile::remove(outputSrt);
        if (!QFile::rename(generated, outputSrt)) {
            if (error) *error = QStringLiteral("Whisper produced an SRT file but it could not be moved to the requested output path");
            return false;
        }
    }

    if (!QFileInfo::exists(outputSrt) || QFileInfo(outputSrt).size() <= 0) {
        if (error) *error = QStringLiteral("Whisper completed without producing an SRT file");
        return false;
    }

    return true;
}
}
