#include "project/ProjectRecovery.hpp"
#include "project/ProjectSerializer.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <utility>

namespace ccos::project {

ProjectRecoveryManager::ProjectRecoveryManager(QString root)
    : root_(QDir::cleanPath(std::move(root))) {
    if (!root_.isEmpty()) QDir().mkpath(root_);
}

QString ProjectRecoveryManager::pathFor(const Project& project) const {
    if (root_.isEmpty()) return {};
    const QString id = QString::fromStdString(project.id().toString());
    if (id.isEmpty()) return {};
    return QDir(root_).filePath(id + QStringLiteral(".ccos"));
}

QString ProjectRecoveryManager::latestSnapshotPath() const {
    if (root_.isEmpty()) return {};
    const QDir directory(root_);
    const QFileInfoList snapshots = directory.entryInfoList(
        QStringList() << QStringLiteral("*.ccos"),
        QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
        QDir::Time);
    return snapshots.isEmpty() ? QString() : snapshots.first().absoluteFilePath();
}

bool ProjectRecoveryManager::save(const Project& project, QString* error) const {
    const QString path = pathFor(project);
    if (path.isEmpty()) {
        if (error) *error = QStringLiteral("Recovery directory is unavailable");
        return false;
    }
    return ProjectSerializer::save(project, path, error);
}

bool ProjectRecoveryManager::load(const QString& path, Project* project, QString* error) const {
    if (!project || path.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Recovery path and destination project are required");
        return false;
    }
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        if (error) *error = QStringLiteral("Recovery snapshot is unavailable: %1").arg(path);
        return false;
    }
    return ProjectSerializer::load(*project, path, error);
}

bool ProjectRecoveryManager::remove(const Project& project) const {
    const QString path = pathFor(project);
    return !path.isEmpty() && !QFileInfo::exists(path) ? true : QFile::remove(path);
}

} // namespace ccos::project
