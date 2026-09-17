#include "render/TimelineExporter.hpp"
#include <QProcess>
#include <QStringList>
#include <cmath>

namespace ccos::render {
namespace {
const ccos::media::MediaAsset* findAsset(const ccos::project::Project& project, const ccos::core::Uuid& id) {
    for (const auto& asset : project.assets()) {
        if (asset.id() == id) return &asset;
    }
    return nullptr;
}

QString secondsText(ccos::core::Time value) {
    return QString::number(value.seconds(), 'f', 9);
}
}

bool TimelineExporter::exportContiguousVideo(const ccos::project::Project& project,
                                             const QString& outputPath,
                                             const ExportSettings& settings,
                                             const QString& executable,
                                             QString* error) {
    if (outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Output path is required");
        return false;
    }

    const ccos::timeline::Track* videoTrack = nullptr;
    for (const auto& track : project.timeline().tracks()) {
        if (track.type() == ccos::timeline::TrackType::Video) {
            videoTrack = &track;
            break;
        }
    }
    if (videoTrack == nullptr || videoTrack->clips().empty()) {
        if (error) *error = QStringLiteral("The project has no video clips to export");
        return false;
    }

    const auto& clips = videoTrack->clips();
    ccos::core::Time expectedStart;
    for (const auto& clip : clips) {
        if (clip.start() != expectedStart) {
            if (error) *error = QStringLiteral("Timeline export currently requires contiguous clips starting at 0");
            return false;
        }
        const auto* asset = findAsset(project, clip.assetId());
        if (asset == nullptr || asset->path().isEmpty()) {
            if (error) *error = QStringLiteral("A timeline clip references missing media");
            return false;
        }
        expectedStart = expectedStart + clip.duration();
    }

    QProcess process;
    QStringList args;
    args << QStringLiteral("-y");
    for (const auto& clip : clips) args << QStringLiteral("-i") << findAsset(project, clip.assetId())->path();

    QString filter;
    QStringList videoLabels;
    QStringList audioLabels;
    for (int i = 0; i < static_cast<int>(clips.size()); ++i) {
        const auto& clip = clips[static_cast<std::size_t>(i)];
        const auto* asset = findAsset(project, clip.assetId());
        const QString vLabel = QStringLiteral("v%1").arg(i);
        const QString aLabel = QStringLiteral("a%1").arg(i);
        filter += QStringLiteral("[%1:v]trim=start=%2:end=%3,setpts=PTS-STARTPTS,scale=%4:%5:force_original_aspect_ratio=decrease,pad=%4:%5:(ow-iw)/2:(oh-ih)/2[%6];")
            .arg(i)
            .arg(secondsText(clip.sourceIn()))
            .arg(secondsText(clip.sourceOut()))
            .arg(settings.width)
            .arg(settings.height)
            .arg(vLabel);
        videoLabels << QStringLiteral("[%1]").arg(vLabel);

        if (asset->metadata().audioChannels > 0 || !asset->metadata().audioCodec.isEmpty()) {
            filter += QStringLiteral("[%1:a]atrim=start=%2:end=%3,asetpts=PTS-STARTPTS[%4];")
                .arg(i)
                .arg(secondsText(clip.sourceIn()))
                .arg(secondsText(clip.sourceOut()))
                .arg(aLabel);
        } else {
            filter += QStringLiteral("anullsrc=channel_layout=stereo:sample_rate=48000:d=%1[%2];")
                .arg(secondsText(clip.duration()))
                .arg(aLabel);
        }
        audioLabels << QStringLiteral("[%1]").arg(aLabel);
    }

    filter += videoLabels.join(QString()) + audioLabels.join(QString()) + QStringLiteral("concat=n=%1:v=1:a=1[outv][outa]").arg(clips.size());
    args << QStringLiteral("-filter_complex") << filter
         << QStringLiteral("-map") << QStringLiteral("[outv]")
         << QStringLiteral("-map") << QStringLiteral("[outa]")
         << QStringLiteral("-r") << QString::number(settings.fps, 'f', 3)
         << QStringLiteral("-c:v") << settings.videoCodec
         << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(settings.videoBitrateKbps)
         << QStringLiteral("-c:a") << settings.audioCodec
         << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(settings.audioBitrateKbps)
         << outputPath;

    process.start(executable, args);
    if (!process.waitForStarted(3000)) {
        if (error) *error = QStringLiteral("Unable to start ffmpeg: %1").arg(process.errorString());
        return false;
    }
    if (!process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(process.readAllStandardError());
        if (error && error->isEmpty()) *error = QStringLiteral("Timeline export failed");
        return false;
    }
    return true;
}
}
