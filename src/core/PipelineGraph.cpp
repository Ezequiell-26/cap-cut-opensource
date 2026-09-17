#include "core/PipelineGraph.hpp"

#include <QtConcurrent>

#include <taskflow/taskflow.hpp>

#include <algorithm>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ccos::core {

struct PipelineGraph::Impl {
    struct Definition {
        TaskFunction function;
    };

    std::unordered_map<std::string, Definition> tasks;
    std::vector<std::pair<std::string, std::string>> dependencies;
    mutable std::mutex mutex;
};

namespace {

using TaskDefinition = std::pair<std::string, PipelineGraph::TaskFunction>;
using Dependency = std::pair<std::string, std::string>;

bool containsCycle(const std::vector<TaskDefinition>& definitions,
                   const std::vector<Dependency>& dependencies) {
    std::unordered_set<std::string> ids;
    ids.reserve(definitions.size());
    for (const auto& [id, _] : definitions) ids.insert(id);

    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    for (const auto& [from, to] : dependencies) {
        if (ids.contains(from) && ids.contains(to)) adjacency[from].push_back(to);
    }

    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    std::function<bool(const std::string&)> visit = [&](const std::string& id) {
        if (visiting.contains(id)) return true;
        if (visited.contains(id)) return false;

        visiting.insert(id);
        const auto it = adjacency.find(id);
        if (it != adjacency.end()) {
            for (const auto& next : it->second) {
                if (visit(next)) return true;
            }
        }
        visiting.erase(id);
        visited.insert(id);
        return false;
    };

    for (const auto& [id, _] : definitions) {
        if (visit(id)) return true;
    }
    return false;
}

bool runSnapshot(std::vector<TaskDefinition> definitions,
                 const std::vector<Dependency>& dependencies,
                 QString* error) {
    if (definitions.empty()) {
        if (error) *error = QStringLiteral("Pipeline graph contains no tasks");
        return false;
    }

    std::sort(definitions.begin(), definitions.end(),
              [](const TaskDefinition& lhs, const TaskDefinition& rhs) {
                  return lhs.first < rhs.first;
              });

    std::unordered_map<std::string, std::size_t> indexes;
    indexes.reserve(definitions.size());
    for (std::size_t i = 0; i < definitions.size(); ++i) indexes.emplace(definitions[i].first, i);

    for (const auto& [from, to] : dependencies) {
        if (!indexes.contains(from) || !indexes.contains(to)) {
            if (error) *error = QStringLiteral("Pipeline dependency references an unknown task");
            return false;
        }
    }

    if (containsCycle(definitions, dependencies)) {
        if (error) *error = QStringLiteral("Pipeline graph contains a cycle");
        return false;
    }

    std::vector<std::vector<std::size_t>> prerequisites(definitions.size());
    for (const auto& [from, to] : dependencies) {
        prerequisites.at(indexes.at(to)).push_back(indexes.at(from));
    }

    taskflow::Taskflow flow;
    std::vector<taskflow::Task> handles;
    std::vector<unsigned char> results(definitions.size(), 0U);
    handles.reserve(definitions.size());

    for (std::size_t i = 0; i < definitions.size(); ++i) {
        handles.push_back(flow.emplace([&, i]() {
            for (const std::size_t prerequisite : prerequisites.at(i)) {
                if (results.at(prerequisite) == 0U) {
                    results[i] = 0U;
                    return;
                }
            }
            results[i] = definitions[i].second() ? 1U : 0U;
        }));
    }

    for (const auto& [from, to] : dependencies) {
        handles.at(indexes.at(from)).precede(handles.at(indexes.at(to)));
    }

    taskflow::Executor executor;
    executor.run(flow).wait();

    for (std::size_t i = 0; i < results.size(); ++i) {
        if (results[i] == 0U) {
            if (error) *error = QStringLiteral("Pipeline task failed: %1")
                .arg(QString::fromStdString(definitions[i].first));
            return false;
        }
    }
    return true;
}

} // namespace

PipelineGraph::PipelineGraph()
    : impl_(std::make_unique<Impl>()) {}

PipelineGraph::~PipelineGraph() = default;

bool PipelineGraph::addTask(const QString& id, TaskFunction function) {
    const std::string key = id.trimmed().toStdString();
    if (key.empty() || !function) return false;

    std::lock_guard lock(impl_->mutex);
    if (impl_->tasks.contains(key)) return false;
    impl_->tasks.emplace(key, Impl::Definition{std::move(function)});
    return true;
}

bool PipelineGraph::addDependency(const QString& prerequisite, const QString& dependent) {
    const std::string from = prerequisite.trimmed().toStdString();
    const std::string to = dependent.trimmed().toStdString();
    if (from.empty() || to.empty() || from == to) return false;

    std::lock_guard lock(impl_->mutex);
    if (!impl_->tasks.contains(from) || !impl_->tasks.contains(to)) return false;
    for (const auto& dependency : impl_->dependencies) {
        if (dependency.first == from && dependency.second == to) return true;
    }

    impl_->dependencies.emplace_back(from, to);
    std::vector<TaskDefinition> definitions;
    definitions.reserve(impl_->tasks.size());
    for (const auto& [id, definition] : impl_->tasks) definitions.emplace_back(id, definition.function);
    if (containsCycle(definitions, impl_->dependencies)) {
        impl_->dependencies.pop_back();
        return false;
    }
    return true;
}

bool PipelineGraph::validate(QString* error) const {
    std::lock_guard lock(impl_->mutex);
    if (impl_->tasks.empty()) {
        if (error) *error = QStringLiteral("Pipeline graph contains no tasks");
        return false;
    }

    std::vector<TaskDefinition> definitions;
    definitions.reserve(impl_->tasks.size());
    for (const auto& [id, definition] : impl_->tasks) definitions.emplace_back(id, definition.function);

    for (const auto& [from, to] : impl_->dependencies) {
        if (!impl_->tasks.contains(from) || !impl_->tasks.contains(to)) {
            if (error) *error = QStringLiteral("Pipeline dependency references an unknown task");
            return false;
        }
    }

    if (containsCycle(definitions, impl_->dependencies)) {
        if (error) *error = QStringLiteral("Pipeline graph contains a cycle");
        return false;
    }
    return true;
}

bool PipelineGraph::run(QString* error) {
    std::vector<TaskDefinition> definitions;
    std::vector<Dependency> dependencies;
    {
        std::lock_guard lock(impl_->mutex);
        definitions.reserve(impl_->tasks.size());
        for (const auto& [id, definition] : impl_->tasks) definitions.emplace_back(id, definition.function);
        dependencies = impl_->dependencies;
    }
    return runSnapshot(std::move(definitions), dependencies, error);
}

QFuture<bool> PipelineGraph::runAsync() {
    std::vector<TaskDefinition> definitions;
    std::vector<Dependency> dependencies;
    {
        std::lock_guard lock(impl_->mutex);
        definitions.reserve(impl_->tasks.size());
        for (const auto& [id, definition] : impl_->tasks) definitions.emplace_back(id, definition.function);
        dependencies = impl_->dependencies;
    }

    return QtConcurrent::run([definitions = std::move(definitions), dependencies = std::move(dependencies)]() mutable {
        return runSnapshot(std::move(definitions), dependencies, nullptr);
    });
}

QStringList PipelineGraph::taskIds() const {
    std::lock_guard lock(impl_->mutex);
    QStringList result;
    result.reserve(static_cast<qsizetype>(impl_->tasks.size()));
    for (const auto& [id, _] : impl_->tasks) result.append(QString::fromStdString(id));
    result.sort();
    return result;
}

void PipelineGraph::clear() {
    std::lock_guard lock(impl_->mutex);
    impl_->tasks.clear();
    impl_->dependencies.clear();
}

} // namespace ccos::core
