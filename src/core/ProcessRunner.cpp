#include "core/ProcessRunner.hpp"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <algorithm>

Q_LOGGING_CATEGORY(ccos_core_process, "ccos.core.process")

namespace ccos::core {

QString ProcessResult::errorMessage() const {
    if (!started) {
        return standardError.isEmpty() ? QStringLiteral("Process failed to start")
                                       : QString::fromUtf8(standardError);
    }
    if (cancelled) return QStringLiteral("Process cancelled by user");
    if (timedOut) return QStringLiteral("Process timed out");
    if (exitStatus == QProcess::CrashExit) return QStringLiteral("Process crashed");
    if (exitCode != 0) {
        const QString details = QString::fromUtf8(standardError).trimmed();
        return details.isEmpty() ? QStringLiteral("Process exited with code %1").arg(exitCode) : details;
    }
    return {};
}

ProcessRunner::ProcessRunner(QObject* parent)
    : QObject(parent) {}

ProcessRunner::~ProcessRunner() {
    cancelAll();
}

bool ProcessRunner::validateExecutable(const QString& executable) {
    const QString value = executable.trimmed();
    if (value.isEmpty()) return false;

    const QFileInfo info(value);
    if (info.isAbsolute() || value.contains(QDir::separator()) || value.contains(QLatin1Char('/')) || value.contains(QLatin1Char('\\'))) {
        return info.exists() && info.isFile() && info.isExecutable();
    }

    return !QStandardPaths::findExecutable(value).isEmpty();
}

void ProcessRunner::appendBounded(QByteArray& destination, const QByteArray& data, qint64 maxSize,
                                  bool& truncated) {
    if (data.isEmpty() || maxSize <= 0) {
        if (!data.isEmpty()) truncated = true;
        return;
    }

    const qint64 remaining = maxSize - destination.size();
    if (remaining <= 0) {
        truncated = true;
        return;
    }

    const qsizetype toAppend = std::min<qsizetype>(data.size(), static_cast<qsizetype>(remaining));
    destination.append(data.constData(), toAppend);
    if (toAppend < data.size()) truncated = true;
}

ProcessResult ProcessRunner::runProcess(const ProcessConfig& config, const ControlPtr& control) {
    ProcessResult result;
    result.executable = config.executable;
    result.arguments = config.arguments;

    const auto timeoutMs = std::max<qint64>(0, config.timeout.count());
    const auto startupTimeoutMs = std::max<qint64>(0, config.startupTimeout.count());

    if (!validateExecutable(config.executable)) {
        result.standardError = QStringLiteral("Executable not found or not executable: %1")
                                   .arg(config.executable)
                                   .toUtf8();
        return result;
    }

    QProcess process;
    process.setProgram(config.executable);
    process.setArguments(config.arguments);

    if (!config.workingDirectory.isEmpty()) {
        const QFileInfo workDir(config.workingDirectory);
        if (!workDir.exists() || !workDir.isDir()) {
            result.standardError = QStringLiteral("Working directory does not exist: %1")
                                       .arg(config.workingDirectory)
                                       .toUtf8();
            return result;
        }
        process.setWorkingDirectory(config.workingDirectory);
    }

    if (!config.environment.isEmpty()) process.setProcessEnvironment(config.environment);

    QElapsedTimer timer;
    timer.start();
    process.start(QIODevice::ReadOnly);

    if (!process.waitForStarted(static_cast<int>(std::min<qint64>(startupTimeoutMs, std::numeric_limits<int>::max())))) {
        result.standardError = process.errorString().toUtf8();
        result.durationMs = timer.elapsed();
        return result;
    }

    result.started = true;
    bool finished = false;

    while (!finished) {
        if (control && control->cancelled.load(std::memory_order_relaxed)) {
            result.cancelled = true;
            process.kill();
            process.waitForFinished(1000);
            finished = true;
        } else if (timer.elapsed() >= timeoutMs) {
            result.timedOut = true;
            process.kill();
            process.waitForFinished(1000);
            finished = true;
        } else {
            finished = process.waitForFinished(50);
        }

        if (config.readStandardOutput) {
            appendBounded(result.standardOutput, process.readAllStandardOutput(), config.maxOutputSize,
                          result.outputTruncated);
        }
        if (config.readStandardError) {
            appendBounded(result.standardError, process.readAllStandardError(), config.maxOutputSize,
                          result.errorTruncated);
        }

        if (!finished && process.state() == QProcess::NotRunning) finished = true;
    }

    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(1000);
    }

    if (config.readStandardOutput) {
        appendBounded(result.standardOutput, process.readAllStandardOutput(), config.maxOutputSize,
                      result.outputTruncated);
    }
    if (config.readStandardError) {
        appendBounded(result.standardError, process.readAllStandardError(), config.maxOutputSize,
                      result.errorTruncated);
    }

    result.exitStatus = process.exitStatus();
    result.exitCode = process.exitCode();
    result.durationMs = timer.elapsed();
    return result;
}

QFuture<ProcessResult> ProcessRunner::execute(const ProcessConfig& config) {
    if (config.executable.trimmed().isEmpty()) {
        ProcessResult result;
        result.standardError = QByteArrayLiteral("Executable path is required");
        return QtConcurrent::run([result]() { return result; });
    }

    const ControlPtr control(new Control());
    {
        QMutexLocker locker(&m_mutex);
        m_active.append(control);
    }

    emit processStarted(config.executable, config.arguments);

    auto future = QtConcurrent::run([config, control]() {
        return runProcess(config, control);
    });

    auto* watcher = new QFutureWatcher<ProcessResult>(this);
    connect(watcher, &QFutureWatcher<ProcessResult>::finished, this,
            [this, watcher, control, config]() {
                const ProcessResult result = watcher->result();
                removeControl(control);
                watcher->deleteLater();
                emit processFinished(result);
                if (!result.isSuccess()) emit processError(result.errorMessage(), config);
            });
    watcher->setFuture(future);
    return future;
}

ProcessResult ProcessRunner::executeSync(const ProcessConfig& config) {
    const ControlPtr control(new Control());
    return runProcess(config, control);
}

void ProcessRunner::cancelAll() {
    QMutexLocker locker(&m_mutex);
    for (const auto& control : m_active) {
        if (control) control->cancelled.store(true, std::memory_order_relaxed);
    }
}

void ProcessRunner::removeControl(const ControlPtr& control) {
    QMutexLocker locker(&m_mutex);
    m_active.removeAll(control);
}

} // namespace ccos::core
