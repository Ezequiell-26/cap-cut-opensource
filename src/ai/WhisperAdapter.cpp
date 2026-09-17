#include "ai/WhisperAdapter.hpp"
#include <QFileInfo>
#include <QProcess>

namespace ccos::ai {
bool WhisperAdapter::transcribeToSrt(const QString& mediaPath, const QString& outputSrt,
                                    const QString& executable, const QString& model, QString* error) {
    if (mediaPath.isEmpty() || outputSrt.isEmpty()) { if (error) *error = QStringLiteral("Media and output paths are required"); return false; }
    QStringList args{QStringLiteral("-m"), model, QStringLiteral("-f"), mediaPath, QStringLiteral("-osrt"), QStringLiteral("-of"), QFileInfo(outputSrt).path() + QLatin1Char('/') + QFileInfo(outputSrt).completeBaseName()};
    if (model.isEmpty()) args.removeFirst(), args.removeFirst();
    QProcess p;
    p.start(executable, args);
    if (!p.waitForStarted(3000)) { if (error) *error = QStringLiteral("Unable to start whisper: %1").arg(p.errorString()); return false; }
    if (!p.waitForFinished(-1) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(p.readAllStandardError());
        return false;
    }
    const QString generated = QFileInfo(outputSrt).path() + QLatin1Char('/') + QFileInfo(outputSrt).completeBaseName() + QStringLiteral(".srt");
    if (generated != outputSrt && QFileInfo::exists(generated)) QFile::rename(generated, outputSrt);
    if (!QFileInfo::exists(outputSrt)) { if (error) *error = QStringLiteral("Whisper completed but did not produce an SRT file"); return false; }
    return true;
}
}
