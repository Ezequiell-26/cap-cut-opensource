#pragma once
#include "project/Project.hpp"
#include <QJsonObject>
#include <QString>

namespace ccos::api {

class EditorApi {
public:
    static QJsonObject inspect(const ccos::project::Project& project);
    static QJsonObject validate(const ccos::project::Project& project);
    static QJsonObject hardwareCapabilities(const QString& ffmpegExecutable = QStringLiteral("ffmpeg"));
    static QJsonObject exportPresets();
    static QJsonObject culturalMediaProviders();
    static QJsonObject command(ccos::project::Project& project,
                               const QJsonObject& request,
                               const QString& ffmpegExecutable = QStringLiteral("ffmpeg"));
};

} // namespace ccos::api
