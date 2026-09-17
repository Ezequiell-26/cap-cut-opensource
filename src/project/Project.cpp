#include "project/Project.hpp"

#include <QFileInfo>

namespace ccos::project {

Project::Project()
    : Project(QStringLiteral("Untitled Project")) {}

Project::Project(QString name)
    : id_()
    , name_(std::move(name)) {
    if (name_.isEmpty()) name_ = QStringLiteral("Untitled Project");
}

Project::Project(ccos::core::Uuid id, QString name)
    : id_(std::move(id))
    , name_(std::move(name)) {
    if (name_.isEmpty()) name_ = QStringLiteral("Untitled Project");
    if (id_.isNull()) id_ = ccos::core::Uuid();
}

int Project::relinkAsset(const ccos::core::Uuid& id, const QString& newPath) {
    if (newPath.isEmpty() || !QFileInfo::exists(newPath)) return 0;
    for (auto& asset : assets_) {
        if (asset.id() == id) {
            asset.setPath(newPath);
            return 1;
        }
    }
    return 0;
}

QStringList Project::missingAssetPaths() const {
    QStringList missing;
    for (const auto& asset : assets_) {
        if (!asset.path().isEmpty() && !QFileInfo::exists(asset.path())) missing.append(asset.path());
    }
    return missing;
}

} // namespace ccos::project
