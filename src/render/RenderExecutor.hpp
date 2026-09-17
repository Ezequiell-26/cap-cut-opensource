#pragma once
#include "project/Project.hpp"
#include "render/ExportSettings.hpp"
#include <QObject>
#include <QProcess>

namespace ccos::render {
class RenderExecutor final : public QObject {
    Q_OBJECT
public:
    explicit RenderExecutor(QObject* parent = nullptr);
    bool start(const ccos::project::Project& project, const QString& output,
               const ExportSettings& settings = {}, const QString& executable = QStringLiteral("ffmpeg"));
    void cancel();
    [[nodiscard]] bool running() const noexcept { return process_.state() != QProcess::NotRunning; }

Q_SIGNALS:
    void progress(double value);
    void message(const QString& text);
    void finished(bool success, const QString& error);

private:
    QProcess process_;
};
}
