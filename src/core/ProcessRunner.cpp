#include "ProcessRunner.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QThread>
#include <QtConcurrent>

Q_LOGGING_CATEGORY(ccos_core_process, "ccos.core.process")

namespace ccos::core {

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent) {}

ProcessRunner::~ProcessRunner() {
    cancelAll();
}

bool ProcessRunner::validateExecutable(const QString &path) {
    if (path.isEmpty()) {
        return false;
    }
    
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        // Intentar buscar en PATH
        QString executableName = path;
#ifdef Q_OS_WIN
        if (!executableName.endsWith(".exe")) {
            executableName += ".exe";
        }
#endif
        QStringList envPaths = QString::qgetenv("PATH").split(QDir::listSeparator());
        for (const QString &envPath : envPaths) {
            QFileInfo check(envPath + QDir::separator() + executableName);
            if (check.exists() && check.isExecutable()) {
                return true;
            }
        }
        return false;
    }
    
    return fileInfo.isExecutable();
}

std::unique_ptr<ProcessRunner::ActiveProcess> ProcessRunner::createProcessInstance(const ProcessConfig &config) {
    auto activeProcess = std::make_unique<ActiveProcess>();
    activeProcess->config = config;
    activeProcess->process = new QProcess(this);
    activeProcess->startTime = QTime::currentTime();
    
    // Configurar proceso
    auto *process = activeProcess->process.data();
    process->setProgram(config.executable);
    process->setArguments(config.arguments);
    
    if (!config.workingDirectory.isEmpty()) {
        process->setWorkingDirectory(config.workingDirectory);
    }
    
    if (!config.environment.isEmpty()) {
        process->setProcessEnvironment(config.environment);
    }
    
    // Conectar señales
    connect(process, &QProcess::started, this, [this, activeProcessPtr = activeProcess.get()]() {
        onProcessStarted();
    });
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, activeProcessPtr = activeProcess.get()](int exitCode, QProcess::ExitStatus exitStatus) {
        onProcessFinished(exitCode, exitStatus);
    });
    
    connect(process, &QProcess::errorOccurred, this, [this, activeProcessPtr = activeProcessPtr.get()](QProcess::ProcessError error) {
        onProcessError(error);
    });
    
    if (config.readStandardOutput) {
        connect(process, &QProcess::readyReadStandardOutput, this, [this, activeProcessPtr = activeProcess.get()]() {
            onReadyReadStandardOutput();
        });
    }
    
    if (config.readStandardError) {
        connect(process, &QProcess::readyReadStandardError, this, [this, activeProcessPtr = activeProcess.get()]() {
            onReadyReadStandardError();
        });
    }
    
    // Configurar timeouts
    activeProcess->timeoutTimer.setSingleShot(true);
    activeProcess->timeoutTimer.setInterval(static_cast<int>(config.timeout.count()));
    connect(&activeProcess->timeoutTimer, &QTimer::timeout, this, [this, activeProcessPtr = activeProcess.get()]() {
        onTimeout();
    });
    
    activeProcess->startupTimer.setSingleShot(true);
    activeProcess->startupTimer.setInterval(static_cast<int>(config.startupTimeout.count()));
    connect(&activeProcess->startupTimer, &QTimer::timeout, this, [this, activeProcessPtr = activeProcess.get()]() {
        onStartupTimeout();
    });
    
    return activeProcess;
}

QFuture<ProcessResult> ProcessRunner::execute(const ProcessConfig &config) {
    // Validaciones de seguridad
    if (config.executable.isEmpty()) {
        qCWarning(ccos_core_process) << "Attempted to execute empty program path";
        ProcessResult error;
        error.exitCode = -1;
        error.standardError = "Empty executable path";
        return QtConcurrent::run([error]() { return error; });
    }
    
    // Prevenir ejecución desde rutas no confiables
    QFileInfo exeInfo(config.executable);
    if (exeInfo.isRelative() && !validateExecutable(config.executable)) {
        qCWarning(ccos_core_process) << "Executable not found or not valid:" << config.executable;
        ProcessResult error;
        error.exitCode = -1;
        error.standardError = "Executable not found: " + config.executable.toLocal8Bit();
        return QtConcurrent::run([error]() { return error; });
    }
    
    auto activeProcess = createProcessInstance(config);
    
    ProcessResult *resultPtr = new ProcessResult();
    resultPtr->executable = config.executable;
    resultPtr->arguments = config.arguments;
    
    QPromise<ProcessResult> promise;
    auto future = promise.future();
    
    // Iniciar proceso
    {
        QMutexLocker locker(&m_processMutex);
        m_activeProcesses.append(std::move(activeProcess));
        auto *activeProc = m_activeProcesses.last().get();
        
        // Iniciar timers
        activeProc->startupTimer.start();
        
        // Iniciar proceso (esto disparará onProcessStarted)
        activeProc->process->start(QIODevice::ReadOnly | QIODevice::Text);
        
        // Esperar a que realmente inicie o falle el startup
        if (!activeProc->process->waitForStarted(static_cast<int>(config.startupTimeout.count()))) {
            qCWarning(ccos_core_process) << "Process failed to start:" << config.executable;
            resultPtr->exitCode = -1;
            resultPtr->standardError = "Failed to start process";
            cleanupProcess(activeProc);
            promise.addResult(*resultPtr);
            delete resultPtr;
            return future;
        }
        
        activeProc->timeoutTimer.start();
    }
    
    // El futuro se completará cuando el proceso termine
    // Esto es una simplificación - en producción usaríamos un mecanismo más robusto
    QtConcurrent::run([this, resultPtr, config]() {
        // Esperar a que el proceso termine (con timeout)
        // La lógica real está en los slots
        return *resultPtr;
    });
    
    return future;
}

ProcessResult ProcessRunner::executeSync(const ProcessConfig &config) {
    // Solo para uso en workers, nunca en UI thread
    if (QThread::currentThread() == qApp->thread()) {
        qCWarning(ccos_core_process) << "WARNING: executeSync called from UI thread! This may freeze the interface.";
    }
    
    ProcessResult result;
    result.executable = config.executable;
    result.arguments = config.arguments;
    
    QProcess process;
    process.setProgram(config.executable);
    process.setArguments(config.arguments);
    
    if (!config.workingDirectory.isEmpty()) {
        process.setWorkingDirectory(config.workingDirectory);
    }
    
    QTime startTime = QTime::currentTime();
    
    // Iniciar con timeout de startup
    if (!process.startDetached()) {
        if (!process.waitForStarted(static_cast<int>(config.startupTimeout.count()))) {
            result.exitCode = -1;
            result.standardError = "Failed to start process";
            result.timedOut = true;
            return result;
        }
    } else {
        process.start(QIODevice::ReadOnly | QIODevice::Text);
    }
    
    // Esperar con timeout
    int timeoutMs = static_cast<int>(config.timeout.count());
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        result.timedOut = true;
        result.exitCode = -1;
        result.standardError = "Process timed out after " + QString::number(timeoutMs) + "ms";
        return result;
    }
    
    result.exitCode = process.exitCode();
    result.exitStatus = process.exitStatus();
    
    if (config.readStandardOutput) {
        result.standardOutput = process.readAllStandardOutput();
    }
    
    if (config.readStandardError) {
        result.standardError = process.readAllStandardError();
    }
    
    result.durationMs = startTime.msecsTo(QTime::currentTime());
    
    return result;
}

void ProcessRunner::cancelAll() {
    QMutexLocker locker(&m_processMutex);
    
    for (auto &activeProcess : m_activeProcesses) {
        if (activeProcess && activeProcess->process) {
            activeProcess->cancelled = true;
            activeProcess->process->kill();
            activeProcess->timeoutTimer.stop();
            activeProcess->startupTimer.stop();
        }
    }
    
    // Limpiar procesos cancelados
    m_activeProcesses.clear();
    
    qCInfo(ccos_core_process) << "All processes cancelled";
}

void ProcessRunner::cleanupProcess(ActiveProcess *activeProcess) {
    if (!activeProcess) return;
    
    activeProcess->timeoutTimer.stop();
    activeProcess->startupTimer.stop();
    
    if (activeProcess->process) {
        activeProcess->process->disconnect();
        activeProcess->process->deleteLater();
    }
    
    // Remover de la lista
    QMutexLocker locker(&m_processMutex);
    m_activeProcesses.erase(
        std::remove_if(m_activeProcesses.begin(), m_activeProcesses.end(),
            [activeProcess](const std::unique_ptr<ActiveProcess> &ptr) {
                return ptr.get() == activeProcess;
            }),
        m_activeProcesses.end()
    );
}

QString ProcessRunner::sanitizeOutput(const QByteArray &output, qint64 maxSize) {
    if (output.size() > maxSize) {
        qCWarning(ccos_core_process) << "Output truncated from" << output.size() << "to" << maxSize << "bytes";
        return QString::fromUtf8(output.left(maxSize));
    }
    return QString::fromUtf8(output);
}

// Slots de implementación
void ProcessRunner::onProcessStarted() {
    auto *process = qobject_cast<QProcess *>(sender());
    if (!process) return;
    
    emit processStarted(process->program(), process->arguments());
}

void ProcessRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    auto *process = qobject_cast<QProcess *>(sender());
    if (!process) return;
    
    // Encontrar el proceso activo correspondiente
    ActiveProcess *activeProcess = nullptr;
    {
        QMutexLocker locker(&m_processMutex);
        for (auto &ap : m_activeProcesses) {
            if (ap->process.data() == process) {
                activeProcess = ap.get();
                break;
            }
        }
    }
    
    if (!activeProcess) {
        qCWarning(ccos_core_process) << "Finished signal from unknown process";
        return;
    }
    
    ProcessResult result;
    result.exitCode = exitCode;
    result.exitStatus = exitStatus;
    result.executable = process->program();
    result.arguments = process->arguments();
    result.standardOutput = activeProcess->outputBuffer;
    result.standardError = activeProcess->errorBuffer;
    result.cancelled = activeProcess->cancelled;
    result.durationMs = activeProcess->startTime.msecsTo(QTime::currentTime());
    
    emit processFinished(result);
    
    cleanupProcess(activeProcess);
}

void ProcessRunner::onProcessError(QProcess::ProcessError error) {
    auto *process = qobject_cast<QProcess *>(sender());
    if (!process) return;
    
    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Failed to start process";
            break;
        case QProcess::Crashed:
            errorMsg = "Process crashed";
            break;
        case QProcess::Timedout:
            errorMsg = "Process timed out";
            break;
        case QProcess::WriteError:
            errorMsg = "Write error";
            break;
        case QProcess::ReadError:
            errorMsg = "Read error";
            break;
        default:
            errorMsg = "Unknown error";
    }
    
    qCWarning(ccos_core_process) << "Process error:" << errorMsg;
    emit processError(errorMsg, ProcessConfig{});
}

void ProcessRunner::onReadyReadStandardOutput() {
    auto *process = qobject_cast<QProcess *>(sender());
    if (!process) return;
    
    // Encontrar buffer correspondiente
    ActiveProcess *activeProcess = nullptr;
    {
        QMutexLocker locker(&m_processMutex);
        for (auto &ap : m_activeProcesses) {
            if (ap->process.data() == process) {
                activeProcess = ap.get();
                break;
            }
        }
    }
    
    if (!activeProcess) return;
    
    QByteArray data = process->readAllStandardOutput();
    
    // Limitar tamaño para evitar DoS
    qint64 available = activeProcess->config.maxOutputSize - activeProcess->outputBuffer.size();
    if (data.size() > available) {
        data = data.left(available);
        process->readAllStandardOutput(); // Descartar resto
        qCWarning(ccos_core_process) << "Output size limit reached, discarding remaining data";
    }
    
    activeProcess->outputBuffer.append(data);
}

void ProcessRunner::onReadyReadStandardError() {
    auto *process = qobject_cast<QProcess *>(sender());
    if (!process) return;
    
    ActiveProcess *activeProcess = nullptr;
    {
        QMutexLocker locker(&m_processMutex);
        for (auto &ap : m_activeProcesses) {
            if (ap->process.data() == process) {
                activeProcess = ap.get();
                break;
            }
        }
    }
    
    if (!activeProcess) return;
    
    QByteArray data = process->readAllStandardError();
    
    qint64 available = activeProcess->config.maxOutputSize - activeProcess->errorBuffer.size();
    if (data.size() > available) {
        data = data.left(available);
        process->readAllStandardError(); // Descartar resto
    }
    
    activeProcess->errorBuffer.append(data);
}

void ProcessRunner::onTimeout() {
    auto *timer = qobject_cast<QTimer *>(sender());
    if (!timer) return;
    
    // Encontrar proceso correspondiente
    ActiveProcess *activeProcess = nullptr;
    {
        QMutexLocker locker(&m_processMutex);
        for (auto &ap : m_activeProcesses) {
            if (&ap->timeoutTimer == timer) {
                activeProcess = ap.get();
                break;
            }
        }
    }
    
    if (!activeProcess || !activeProcess->process) return;
    
    qCWarning(ccos_core_process) << "Process timed out:" << activeProcess->config.executable;
    activeProcess->process->kill();
    activeProcess->timedOut = true;
}

void ProcessRunner::onStartupTimeout() {
    auto *timer = qobject_cast<QTimer *>(sender());
    if (!timer) return;
    
    ActiveProcess *activeProcess = nullptr;
    {
        QMutexLocker locker(&m_processMutex);
        for (auto &ap : m_activeProcesses) {
            if (&ap->startupTimer == timer) {
                activeProcess = ap.get();
                break;
            }
        }
    }
    
    if (!activeProcess || !activeProcess->process) return;
    
    // Si el proceso ya inició, ignorar
    if (activeProcess->process->state() != QProcess::NotRunning) {
        return;
    }
    
    qCWarning(ccos_core_process) << "Process startup timed out:" << activeProcess->config.executable;
    activeProcess->process->kill();
    cleanupProcess(activeProcess);
}

} // namespace ccos::core
