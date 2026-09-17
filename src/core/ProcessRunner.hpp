#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QPointer>
#include <memory>
#include <functional>
#include <chrono>

namespace ccos::core {

/**
 * @brief Resultado estructurado de la ejecución de un proceso externo.
 * 
 * Diseñado para ser inmutable y contener toda la información de diagnóstico.
 */
struct ProcessResult {
    int exitCode = -1;
    QProcess::ExitStatus exitStatus = QProcess::CrashExit;
    QByteArray standardOutput;
    QByteArray standardError;
    bool timedOut = false;
    bool cancelled = false;
    QString executable;
    QStringList arguments;
    qint64 durationMs = 0;
    
    bool isSuccess() const {
        return !timedOut && !cancelled && exitStatus == QProcess::NormalExit && exitCode == 0;
    }
    
    QString errorMessage() const {
        if (timedOut) return "Process timed out";
        if (cancelled) return "Process cancelled by user";
        if (exitStatus == QProcess::CrashExit) return "Process crashed";
        if (exitCode != 0) return QString("Process exited with code %1: %2").arg(exitCode).arg(standardError.constData());
        return {};
    }
};

/**
 * @brief Configuración segura para la ejecución de procesos.
 * 
 * Previene errores comunes como bloqueos indefinidos o inyección de comandos.
 */
struct ProcessConfig {
    QString executable;
    QStringList arguments; // Separados, nunca concatenados
    std::chrono::milliseconds timeout{30000}; // Default 30s
    std::chrono::milliseconds startupTimeout{5000}; // Tiempo máximo para iniciar
    QString workingDirectory;
    QProcessEnvironment environment;
    bool readStandardOutput = true;
    bool readStandardError = true;
    qint64 maxOutputSize = 10 * 1024 * 1024; // 10MB límite para evitar DoS
    
    // Política de reintentos
    int maxRetries = 0;
    std::chrono::milliseconds retryDelay{1000};
    
    // Clasificación para logging y auditoría
    enum class RiskLevel { Low, Medium, High };
    RiskLevel riskLevel = RiskLevel::Low;
};

/**
 * @brief Gestor seguro de procesos externos (FFmpeg, FFprobe, Whisper, etc.)
 * 
 * CARACTERÍSTICAS DE SEGURIDAD:
 * - Timeouts estrictos para prevenir bloqueos
 * - Cancelación segura
 * - Límites de memoria en output
 * - Logging estructurado
 * - Sin construcción de strings de shell
 * - Clasificación de fallos
 */
class ProcessRunner : public QObject {
    Q_OBJECT

public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    /**
     * @brief Ejecuta un proceso de forma asíncrona.
     * @param config Configuración del proceso
     * @return QFuture<ProcessResult> para observar el resultado
     */
    QFuture<ProcessResult> execute(const ProcessConfig &config);

    /**
     * @brief Ejecuta un proceso de forma síncrona (SOLO para operaciones rápidas en workers).
     * @warning No usar en el hilo principal de UI.
     */
    ProcessResult executeSync(const ProcessConfig &config);

    /**
     * @brief Cancela todos los procesos activos.
     */
    void cancelAll();

    /**
     * @brief Verifica si el ejecutable existe y es accesible.
     */
    static bool validateExecutable(const QString &path);

signals:
    void processStarted(const QString &executable, const QStringList &arguments);
    void processFinished(const ProcessResult &result);
    void processError(const QString &error, const ProcessConfig &config);
    void progressUpdated(int percent, const QString &status);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onTimeout();
    void onStartupTimeout();

private:
    struct ActiveProcess {
        QPointer<QProcess> process;
        QTimer timeoutTimer;
        QTimer startupTimer;
        ProcessConfig config;
        QTime startTime;
        QByteArray outputBuffer;
        QByteArray errorBuffer;
        bool cancelled = false;
    };

    std::unique_ptr<ActiveProcess> createProcessInstance(const ProcessConfig &config);
    void cleanupProcess(ActiveProcess *activeProcess);
    QString sanitizeOutput(const QByteArray &output, qint64 maxSize);
    
    QList<std::unique_ptr<ActiveProcess>> m_activeProcesses;
    mutable QMutex m_processMutex;
    
    friend class ProcessRunnerTest;
};

} // namespace ccos::core
