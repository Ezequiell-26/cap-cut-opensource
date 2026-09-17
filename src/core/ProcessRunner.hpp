#pragma once

#include <QObject>
#include <QByteArray>
#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QMutex>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

#include <atomic>
#include <chrono>

namespace ccos::core {

struct ProcessResult {
    int exitCode = -1;
    QProcess::ExitStatus exitStatus = QProcess::CrashExit;
    QByteArray standardOutput;
    QByteArray standardError;
    bool started = false;
    bool timedOut = false;
    bool cancelled = false;
    bool outputTruncated = false;
    bool errorTruncated = false;
    QString executable;
    QStringList arguments;
    qint64 durationMs = 0;

    [[nodiscard]] bool isSuccess() const noexcept {
        return started && !timedOut && !cancelled && exitStatus == QProcess::NormalExit && exitCode == 0;
    }

    [[nodiscard]] QString errorMessage() const;
};

struct ProcessConfig {
    QString executable;
    QStringList arguments;
    std::chrono::milliseconds timeout{30'000};
    std::chrono::milliseconds startupTimeout{5'000};
    QString workingDirectory;
    QProcessEnvironment environment;
    bool readStandardOutput = true;
    bool readStandardError = true;
    qint64 maxOutputSize = 10 * 1024 * 1024;

    enum class RiskLevel { Low, Medium, High };
    RiskLevel riskLevel = RiskLevel::Low;
};

class ProcessRunner final : public QObject {
    Q_OBJECT

public:
    explicit ProcessRunner(QObject* parent = nullptr);
    ~ProcessRunner() override;

    [[nodiscard]] QFuture<ProcessResult> execute(const ProcessConfig& config);
    [[nodiscard]] ProcessResult executeSync(const ProcessConfig& config);

    void cancelAll();
    [[nodiscard]] static bool validateExecutable(const QString& executable);

signals:
    void processStarted(const QString& executable, const QStringList& arguments);
    void processFinished(const ProcessResult& result);
    void processError(const QString& error, const ProcessConfig& config);

private:
    struct Control {
        std::atomic_bool cancelled{false};
    };

    using ControlPtr = QSharedPointer<Control>;

    static ProcessResult runProcess(const ProcessConfig& config, const ControlPtr& control);
    static void appendBounded(QByteArray& destination, const QByteArray& data, qint64 maxSize,
                              bool& truncated);
    void removeControl(const ControlPtr& control);

    mutable QMutex m_mutex;
    QList<ControlPtr> m_active;
};

} // namespace ccos::core
