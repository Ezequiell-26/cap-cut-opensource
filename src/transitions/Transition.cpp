#include "transitions/Transition.hpp"

namespace ccos::transitions {
QString Transition::ffmpegName() const {
    switch (type_) {
    case TransitionType::Cut: return QStringLiteral("fade");
    case TransitionType::Fade: return QStringLiteral("fade");
    case TransitionType::Dissolve: return QStringLiteral("fade");
    case TransitionType::DipToBlack: return QStringLiteral("fadeblack");
    case TransitionType::Wipe: return QStringLiteral("wipeleft");
    case TransitionType::Slide: return QStringLiteral("slideright");
    case TransitionType::Zoom: return QStringLiteral("zoomin");
    }
    return QStringLiteral("fade");
}

QString Transition::ffmpegFilter(double durationSeconds, double offsetSeconds) const {
    if (type_ == TransitionType::Cut) return {};
    return QStringLiteral("xfade=transition=%1:duration=%2:offset=%3")
        .arg(ffmpegName()).arg(durationSeconds, 0, 'f', 3).arg(offsetSeconds, 0, 'f', 3);
}
}
