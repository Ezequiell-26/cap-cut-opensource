#include "project/Project.hpp"
namespace ccos::project {
Project::Project() : name_(QStringLiteral("Untitled Project")) {}
Project::Project(QString name) : name_(std::move(name)) {}
}
