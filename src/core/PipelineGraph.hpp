#pragma once

#include <QFuture>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

namespace ccos::core {

/**
 * Dependency-aware native C++ task graph built on Taskflow.
 *
 * Nodes are pure application tasks; Project/Timeline mutation must still be
 * performed through validated Commands on the owning application thread.
 */
class PipelineGraph final {
public:
    using TaskFunction = std::function<bool()>;

    PipelineGraph();
    ~PipelineGraph();

    PipelineGraph(const PipelineGraph&) = delete;
    PipelineGraph& operator=(const PipelineGraph&) = delete;

    [[nodiscard]] bool addTask(const QString& id, TaskFunction function);
    [[nodiscard]] bool addDependency(const QString& prerequisite, const QString& dependent);
    [[nodiscard]] bool validate(QString* error = nullptr) const;

    [[nodiscard]] bool run(QString* error = nullptr);
    [[nodiscard]] QFuture<bool> runAsync();

    [[nodiscard]] QStringList taskIds() const;
    void clear();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ccos::core
