#pragma once
#include "project/Project.hpp"
#include <QString>

namespace ccos::project {
class ProjectSerializer {
public:
    static bool save(const Project& project, const QString& path, QString* error = nullptr);
    static bool load(Project& project, const QString& path, QString* error = nullptr);
};
}
