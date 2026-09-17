#pragma once

#include "project/Project.hpp"

#include <QString>

namespace ccos::project {

class ProjectRecoveryManager {
public:
    explicit ProjectRecoveryManager(QString root);

    [[nodiscard]] QString pathFor(const Project& project) const;
    [[nodiscard]] QString latestSnapshotPath() const;

    bool save(const Project& project, QString* error = nullptr) const;
    bool load(const QString& path, Project* project, QString* error = nullptr) const;
    bool remove(const Project& project) const;

private:
    QString root_;
};

} // namespace ccos::project
