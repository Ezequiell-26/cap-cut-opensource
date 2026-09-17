#pragma once
#include <QString>
#include <QVector>
#include <functional>
#include <utility>

namespace ccos::render {

struct RenderContext {
    qint64 frameNumber = 0;
    qint64 timeMs = 0;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;
    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual bool render(RenderContext& context, QString* error = nullptr) = 0;
};

class RenderGraph {
public:
    void addPass(std::unique_ptr<RenderPass> pass) {
        if (pass) passes_.push_back(std::move(pass));
    }

    [[nodiscard]] bool execute(RenderContext& context, QString* error = nullptr) const {
        for (const auto& pass : passes_) {
            if (!pass->render(context, error)) return false;
        }
        return true;
    }

    [[nodiscard]] qsizetype size() const noexcept { return passes_.size(); }

private:
    QVector<std::unique_ptr<RenderPass>> passes_;
};

} // namespace ccos::render
