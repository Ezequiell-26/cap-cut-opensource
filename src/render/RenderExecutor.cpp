#include "render/RenderExecutor.hpp"
#include "render/TimelineCompositor.hpp"
#include <QRegularExpression>

namespace ccos::render {
RenderExecutor::RenderExecutor(QObject* parent) : QObject(parent) {
    process_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&process_, &QProcess::readyRead, this, [this] {
        const QString text = QString::fromLocal8Bit(process_.readAll());
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        static const QRegularExpression progressRe(QStringLiteral("out_time_ms=(\\d+)"));
        static qint64 lastMs = 0;
        for (const auto& line : lines) {
            const auto m = progressRe.match(line);
            if (m.hasMatch()) {
                lastMs = m.captured(1).toLongLong();
                emit progress(lastMs / 1000000.0);
            }
            if (!line.startsWith(QStringLiteral("out_time_ms"))) emit message(line.trimmed());
        }
    });
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (process_.error() != QProcess::UnknownError) emit finished(false, process_.errorString());
    });
    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                if (code == 0 && status == QProcess::NormalExit) emit finished(true, {});
                else emit finished(false, QString::fromLocal8Bit(process_.readAllStandardError()));
            });
}

bool RenderExecutor::start(const ccos::project::Project& project, const QString& output,
                           const ExportSettings& settings, const QString& executable) {
    if (running() || output.isEmpty()) return false;
    QStringList inputs; QString filter; QString video; QString audio; QString error;
    if (!TimelineCompositor::build(project, settings, inputs, filter, video, audio, &error)) {
        emit finished(false, error); return false;
    }
    QStringList args{QStringLiteral("-hide_banner"), QStringLiteral("-progress"), QStringLiteral("pipe:1"), QStringLiteral("-nostats"), QStringLiteral("-y")};
    for (const auto& input : inputs) args << QStringLiteral("-i") << input;
    args << QStringLiteral("-filter_complex") << filter << QStringLiteral("-map") << video << QStringLiteral("-map") << audio
         << QStringLiteral("-c:v") << settings.videoCodec << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(settings.videoBitrateKbps)
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p") << QStringLiteral("-r") << QString::number(settings.fps, 'f', 3)
         << QStringLiteral("-c:a") << settings.audioCodec << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(settings.audioBitrateKbps)
         << QStringLiteral("-shortest") << output;
    process_.start(executable, args);
    return process_.waitForStarted(3000);
}

void RenderExecutor::cancel() { if (running()) process_.kill(); }
}
