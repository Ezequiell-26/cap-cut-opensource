#include "render/TimelineCompositor.hpp"
#include "effects/BuiltinEffects.hpp"
#include "render/TextComposer.hpp"
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <QVector>

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
                const auto& t = clip.transform();
                const double cl = std::clamp(t.cropLeft, 0.0, 0.49), ct = std::clamp(t.cropTop, 0.0, 0.49), cr = std::clamp(t.cropRight, 0.0, 0.49), cb = std::clamp(t.cropBottom, 0.0, 0.49);
                QString vf = QStringLiteral("[%1:v]trim=start=%2:end=%3,setpts=(PTS-STARTPTS)/%4")
                    .arg(inputIndex).arg(timeText(clip.sourceIn())).arg(timeText(clip.sourceOut())).arg(speed);
                if (cl > 0.0 || ct > 0.0 || cr > 0.0 || cb > 0.0)
                    vf += QStringLiteral(",crop=iw*%1:ih*%2:iw*%3:ih*%4").arg(1.0-cl-cr,0,'f',6).arg(1.0-ct-cb,0,'f',6).arg(cl,0,'f',6).arg(ct,0,'f',6);
                if (t.flipHorizontal) vf += QStringLiteral(",hflip");
                if (t.flipVertical) vf += QStringLiteral(",vflip");
                vf += QStringLiteral(",scale=iw*%1:ih*%2,rotate=%3*PI/180:fillcolor=black@0").arg(t.scaleX).arg(t.scaleY).arg(t.rotation);
                vf += QStringLiteral(",scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2").arg(settings.width).arg(settings.height);
                const double opacity = std::clamp(t.opacity, 0.0, 1.0);
                if (opacity < 0.999) vf += QStringLiteral(",colorchannelmixer=aa=%1").arg(opacity,0,'f',3);
                for (const auto& effect : clip.effects()) { const QString f = ccos::effects::BuiltinEffects::ffmpegFilter(effect); if (!f.isEmpty()) vf += QStringLiteral(",") + f; }
                const double transitionSeconds = static_cast<double>(clip.transitionInDurationMs()) / 1000.0;
                if (transitionSeconds > 0.0 && clip.transitionInId() != QStringLiteral("cut")) {
                    if (clip.transitionInId() == QStringLiteral("fade") || clip.transitionInId() == QStringLiteral("dissolve"))
                        vf += QStringLiteral(",fade=t=in:st=0:d=%1:alpha=1").arg(transitionSeconds,0,'f',3);
                }
                vf += QStringLiteral(",setpts=PTS+%1/TB[%2]").arg(timeText(clip.start())).arg(v);
                filterComplex += vf + QLatin1Char(';');
                videoLayers.push_back({v, t.x, t.y});
            }
            if (asset->metadata().audioChannels > 0 || !asset->metadata().audioCodec.isEmpty()) {
                QString af = QStringLiteral("[%1:a]atrim=start=%2:end=%3,asetpts=PTS-STARTPTS").arg(inputIndex).arg(timeText(clip.sourceIn())).arg(timeText(clip.sourceOut()));
                if (speed != 1.0) {
                    double s = speed;
                    while (s > 2.0) { af += QStringLiteral(",atempo=2.0"); s /= 2.0; }
                    while (s < 0.5) { af += QStringLiteral(",atempo=0.5"); s /= 0.5; }
                    af += QStringLiteral(",atempo=%1").arg(s,0,'f',6);
                }
                const double rawAudioGain = clip.audioGain();
                const double audioGain = std::isfinite(rawAudioGain) ? std::clamp(rawAudioGain, 0.0, 4.0) : 1.0;
                if (clip.audioMuted()) {
                    af += QStringLiteral(",volume=0");
                } else if (std::abs(audioGain - 1.0) > 0.0001) {
                    af += QStringLiteral(",volume=%1").arg(audioGain, 0, 'f', 6);
                }
                af += QStringLiteral(",adelay=%1|%1[a%2]").arg(
                    static_cast<qint64>(std::max(0.0, clip.start().seconds()) * 1000.0)).arg(inputIndex);
                filterComplex += af + QLatin1Char(';'); audioLabels << QStringLiteral("[a%1]").arg(inputIndex);
            }
            ++inputIndex;
        }
    }

    filterComplex += QStringLiteral("color=c=black:s=%1x%2:r=%3:d=86400[canvas];").arg(settings.width).arg(settings.height).arg(settings.fps,0,'f',3);
    QString current = QStringLiteral("[canvas]"); int layer = 0;
    for (const auto& item : videoLayers) {
        const QString out = QStringLiteral("mixv%1").arg(layer++);
        filterComplex += current + QStringLiteral("[%1]overlay=shortest=0:eof_action=pass:x=%2:y=%3[%4];").arg(item.label).arg(item.x,0,'f',2).arg(item.y,0,'f',2).arg(out);
        current = QStringLiteral("[%1]").arg(out);
    }
    int textIndex = 0;
    for (const auto& textLayer : project.textLayers()) {
        const QString output = QStringLiteral("txt%1").arg(textIndex++);
        const QString expression = TextComposer::apply(current, textLayer, output);
        if (expression != current + output) { filterComplex += expression + QLatin1Char(';'); current = QStringLiteral("[%1]").arg(output); }
    }
    videoMap = current;
    if (audioLabels.isEmpty()) { filterComplex += QStringLiteral("anullsrc=channel_layout=stereo:sample_rate=48000:d=86400[aout];"); audioMap = QStringLiteral("[aout]"); }
    else if (audioLabels.size() == 1) audioMap = audioLabels.front();
    else { filterComplex += audioLabels.join(QString()) + QStringLiteral("amix=inputs=%1:duration=longest:dropout_transition=0:normalize=1[aout];").arg(audioLabels.size()); audioMap = QStringLiteral("[aout]"); }
    return true;
}
}
