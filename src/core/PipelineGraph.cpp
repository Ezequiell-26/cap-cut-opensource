#include "core/PipelineGraph.hpp"

#include <QtConcurrent>

#include <taskflow/taskflow.hpp>

#include <mutex>
#include <unordered_map>
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
    return true;
}

bool PipelineGraph::validate(QString* error) const {
    std::lock_guard lock(impl_->mutex);
    if (impl_->tasks.empty()) {
        if (error) *error = QStringLiteral("Pipeline graph contains no tasks");
        return false;
    }

    taskflow::Taskflow flow;
    std::unordered_map<std::string, taskflow::Task> handles;
    handles.reserve(impl_->tasks.size());
    for (const auto& [id, _] : impl_->tasks) handles.emplace(id, flow.emplace([] {}));

    for (const auto& [from, to] : impl_->dependencies) {
        handles.at(from).precede(handles.at(to));
    }

    if (flow.num_strongly_connected_components() != 0) {
        // Taskflow's graph should be acyclic for this abstraction. The exact
        // SCC API may change; use the topological executor as the final check.
    }

    const auto topology = flow.topological_sort();
    if (topology.size() != impl_->tasks.size()) {
        if (error) *error = QStringLiteral("Pipeline graph contains a cycle");
        return false;
    }
    return true;
}

bool PipelineGraph::run(QString* error) {
    std::vector<Impl::Definition> definitions;
    std::vector<std::pair<std::string, std::string>> dependencies;
    std::unordered_map<std::string, std::size_t> indexes;

    {
        std::lock_guard lock(impl_->mutex);
        if (impl_->tasks.empty()) {
            if (error) *error = QStringLiteral("Pipeline graph contains no tasks");
            return false;
        }

        definitions.reserve(impl_->tasks.size());
        for (const auto& [id, definition] : impl_->tasks) {
            indexes.emplace(id, definitions.size());
            definitions.push_back(definition);
        }
        dependencies = impl_->dependencies;
    }

    taskflow::Taskflow flow;
    std::vector<taskflow::Task> handles;
    handles.reserve(definitions.size());
    std::vector<bool> results(definitions.size(), false);
    for (std::size_t i = 0; i < definitions.size(); ++i) {
        handles.push_back(flow.emplace([&, i]() { results[i] = definitions[i].function(); }));
    }

    for (const auto& [from, to] : dependencies) {
        const auto fromIt = indexes.find(from);
        const auto toIt = indexes.find(to);
        if (fromIt == indexes.end() || toIt == indexes.end()) {
            if (error) *error = QStringLiteral("Pipeline dependency references an unknown task");
            return false;
        }
        handles[fromIt->second].precede(handles[toIt->second]);
    }

    taskflow::Executor executor;
    executor.run(flow).wait();

    for (std::size_t i = 0; i < results.size(); ++i) {
        if (!results[i]) {
            if (error) *error = QStringLiteral("Pipeline task failed at index %1").arg(static_cast<qulonglong>(i));
            return false;
        }
    }
    return true;
}

QFuture<bool> PipelineGraph::runAsync() {
    return QtConcurrent::run([this]() {
        return run(nullptr);
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
