#include "render/TimelineCompositor.hpp"
#include "effects/BuiltinEffects.hpp"
#include <QFileInfo>
#include <algorithm>

namespace ccos::render {
namespace {
const ccos::media::MediaAsset* assetFor(const ccos::project::Project& p, const ccos::core::Uuid& id) {
    for (const auto& a : p.assets()) if (a.id() == id) return &a;
    return nullptr;
}
QString timeText(ccos::core::Time t) { return QString::number(std::max(0.0, t.seconds()), 'f', 6); }
struct VideoLayer { QString label; double x = 0.0; double y = 0.0; };
}

bool TimelineCompositor::build(const ccos::project::Project& project,
                               const ExportSettings& settings,
                               QStringList& inputs,
                               QString& filterComplex,
                               QString& videoMap,
                               QString& audioMap,
                               QString* error) {
    inputs.clear(); filterComplex.clear(); videoMap.clear(); audioMap.clear();
    int inputIndex = 0;
    QVector<VideoLayer> videoLayers;
    QStringList audioLabels;

    for (const auto& track : project.timeline().tracks()) {
        for (const auto& clip : track.clips()) {
            const auto* asset = assetFor(project, clip.assetId());
            if (!asset || asset->path().isEmpty() || !QFileInfo::exists(asset->path())) {
                if (error) *error = QStringLiteral("Missing media for clip %1").arg(QString::fromStdString(clip.id().toString()).left(8));
                return false;
            }
            inputs << asset->path();
            const double speed = clip.speed() > 0.0 ? clip.speed() : 1.0;
            if (track.type() == ccos::timeline::TrackType::Video) {
                const QString v = QStringLiteral("v%1").arg(inputIndex);
                QString vf = QStringLiteral("[%1:v]trim=start=%2:end=%3,setpts=(PTS-STARTPTS)/%4,scale=iw*%5:ih*%6,rotate=%7*PI/180:fillcolor=black@0")
                    .arg(inputIndex).arg(timeText(clip.sourceIn())).arg(timeText(clip.sourceOut())).arg(speed)
                    .arg(clip.transform().scaleX).arg(clip.transform().scaleY).arg(clip.transform().rotation);
                vf += QStringLiteral(",scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2")
                    .arg(settings.width).arg(settings.height);
                const double opacity = std::clamp(clip.transform().opacity, 0.0, 1.0);
                if (opacity < 0.999) vf += QStringLiteral(",colorchannelmixer=aa=%1").arg(opacity, 0, 'f', 3);
                for (const auto& effect : clip.effects()) {
                    const QString f = ccos::effects::BuiltinEffects::ffmpegFilter(effect);
                    if (!f.isEmpty()) vf += QStringLiteral(",") + f;
                }
                vf += QStringLiteral(",setpts=PTS+%1/TB[%2]").arg(timeText(clip.start())).arg(v);
                filterComplex += vf + QLatin1Char(';');
                videoLayers.push_back({v, clip.transform().x, clip.transform().y});
            }
            if (asset->metadata().audioChannels > 0 || !asset->metadata().audioCodec.isEmpty()) {
                QString af = QStringLiteral("[%1:a]atrim=start=%2:end=%3,asetpts=PTS-STARTPTS")
                    .arg(inputIndex).arg(timeText(clip.sourceIn())).arg(timeText(clip.sourceOut()));
                if (speed != 1.0) {
                    double s = speed;
                    while (s > 2.0) { af += QStringLiteral(",atempo=2.0"); s /= 2.0; }
                    while (s < 0.5) { af += QStringLiteral(",atempo=0.5"); s /= 0.5; }
                    af += QStringLiteral(",atempo=%1").arg(s, 0, 'f', 6);
                }
                const qint64 delay = static_cast<qint64>(clip.start().seconds() * 1000.0);
                af += QStringLiteral(",adelay=%1|%1[%2]").arg(delay).arg(QStringLiteral("a%1").arg(inputIndex));
                filterComplex += af + QLatin1Char(';');
                audioLabels << QStringLiteral("[a%1]").arg(inputIndex);
            }
            ++inputIndex;
        }
    }

    filterComplex += QStringLiteral("color=c=black:s=%1x%2:r=%3:d=86400[canvas];")
        .arg(settings.width).arg(settings.height).arg(settings.fps, 0, 'f', 3);
    QString current = QStringLiteral("[canvas]");
    int layer = 0;
    for (const auto& item : videoLayers) {
        const QString out = QStringLiteral("mixv%1").arg(layer++);
        filterComplex += current + QStringLiteral("[%1]overlay=shortest=0:eof_action=pass:x=%2:y=%3[%4];")
            .arg(item.label).arg(item.x, 0, 'f', 2).arg(item.y, 0, 'f', 2).arg(out);
        current = QStringLiteral("[%1]").arg(out);
    }
    videoMap = current;

    if (audioLabels.isEmpty()) {
        filterComplex += QStringLiteral("anullsrc=channel_layout=stereo:sample_rate=48000:d=86400[aout];");
        audioMap = QStringLiteral("[aout]");
    } else if (audioLabels.size() == 1) {
        audioMap = audioLabels.front();
    } else {
        filterComplex += audioLabels.join(QString()) + QStringLiteral("amix=inputs=%1:duration=longest:dropout_transition=0:normalize=1[aout];").arg(audioLabels.size());
        audioMap = QStringLiteral("[aout]");
    }
    return true;
}
}
