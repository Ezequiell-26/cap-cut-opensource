#pragma once
#include "project/Project.hpp"
#include "render/ExportSettings.hpp"
#include <QObject>
#include <QProcess>
#include <QTimer>

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
    static constexpr int kStartupTimeoutMs = 3000;
    static constexpr int kTotalTimeoutMs = 6 * 60 * 60 * 1000;
    static constexpr qsizetype kDiagnosticLimit = 256 * 1024;

    void appendDiagnostic(const QString& text);

    QProcess process_;
    QTimer timeoutTimer_;
    QByteArray diagnosticBuffer_;
};
}
