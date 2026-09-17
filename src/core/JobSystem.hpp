#pragma once

#include <QObject>
#include <QFuture>
#include <QPromise>
#include <QMutex>
#include <QQueue>
#include <QLoggingCategory>
#include <QUuid>
#include <QDateTime>
#include <functional>
#include <memory>
#include <atomic>

namespace ccos::core {

/**
 * @brief Estados posibles de un Job en el sistema.
 */
enum class JobState {
    QUEUED,      // Esperando ejecución
    RUNNING,     // En ejecución
    PAUSED,      // Pausado temporalmente
    CANCELLING,  // En proceso de cancelación
    CANCELLED,   // Cancelado exitosamente
    FAILED,      // Falló con error
    COMPLETED    // Completado exitosamente
};

/**
 * @brief Tipos de Jobs soportados por el sistema.
 */
enum class JobType {
    IMPORT_MEDIA,
    PROBE_MEDIA,
    GENERATE_THUMBNAIL,
    GENERATE_WAVEFORM,
    GENERATE_PROXY,
    RENDER_PREVIEW,
    EXPORT_VIDEO,
    TRANSCRIBE_AUDIO,
    AI_ANALYSIS,
    CACHE_OPERATION,
    CUSTOM
};

/**
 * @brief Información detallada de error para diagnóstico.
 */
struct JobError {
    int code = -1;
    QString message;
    QString details;
    bool recoverable = false;
    bool retryable = false;
    
    static JobError success() { return {}; }
    static JobError failed(const QString &msg, int code = -1, bool retryable = false) {
        JobError err;
        err.code = code;
        err.message = msg;
        err.retryable = retryable;
        return err;
    }
};

/**
 * @brief Configuración de un Job.
 */
struct JobConfig {
    JobType type = JobType::CUSTOM;
    QString id;
    QString name;
    int priority = 5; // 0-10, donde 10 es más prioritario
    std::function<QVariant()> executeFn;
    std::function<void(double)> progressCallback;
    std::function<void(const JobError &)> completionCallback;
    int maxRetries = 1;
    bool cancelable = true;
    QVariant userData;
};

/**
 * @brief Estado completo de un Job.
 */
struct JobStatus {
    QString id;
    JobType type = JobType::CUSTOM;
    QString name;
    JobState state = JobState::QUEUED;
    int priority = 5;
    double progress = 0.0; // 0.0 - 1.0
    QDateTime createdAt;
    QDateTime startedAt;
    QDateTime completedAt;
    JobError error;
    int retryCount = 0;
    int maxRetries = 1;
    bool cancelable = true;
    QVariant result;
    QVariant userData;
    
    QString stateToString() const {
        switch (state) {
            case JobState::QUEUED: return "QUEUED";
            case JobState::RUNNING: return "RUNNING";
            case JobState::PAUSED: return "PAUSED";
            case JobState::CANCELLING: return "CANCELLING";
            case JobState::CANCELLED: return "CANCELLED";
            case JobState::FAILED: return "FAILED";
            case JobState::COMPLETED: return "COMPLETED";
            default: return "UNKNOWN";
        }
    }
    
    QString typeToString() const {
        switch (type) {
            case JobType::IMPORT_MEDIA: return "IMPORT_MEDIA";
            case JobType::PROBE_MEDIA: return "PROBE_MEDIA";
            case JobType::GENERATE_THUMBNAIL: return "GENERATE_THUMBNAIL";
            case JobType::GENERATE_WAVEFORM: return "GENERATE_WAVEFORM";
            case JobType::GENERATE_PROXY: return "GENERATE_PROXY";
            case JobType::RENDER_PREVIEW: return "RENDER_PREVIEW";
            case JobType::EXPORT_VIDEO: return "EXPORT_VIDEO";
            case JobType::TRANSCRIBE_AUDIO: return "TRANSCRIBE_AUDIO";
            case JobType::AI_ANALYSIS: return "AI_ANALYSIS";
            case JobType::CACHE_OPERATION: return "CACHE_OPERATION";
            case JobType::CUSTOM: return "CUSTOM";
            default: return "UNKNOWN";
        }
    }
};

/**
 * @brief Sistema de Jobs para operaciones asíncronas y concurrentes.
 * 
 * CARACTERÍSTICAS:
 * - Cola de prioridad
 * - Cancelación segura
 * - Reintentos automáticos
 * - Observabilidad completa
 * - Sin bloqueo de UI
 * - Prevención de starvation
 */
class JobSystem : public QObject {
    Q_OBJECT

public:
    explicit JobSystem(int maxConcurrentJobs = 4, QObject *parent = nullptr);
    ~JobSystem() override;

    /**
     * @brief Agrega un job a la cola de ejecución.
     * @param config Configuración del job
     * @return ID único del job
     */
    QString enqueue(const JobConfig &config);

    /**
     * @brief Cancela un job específico.
     * @param jobId ID del job a cancelar
     * @return true si la cancelación fue solicitada
     */
    bool cancelJob(const QString &jobId);

    /**
     * @brief Cancela todos los jobs activos.
     */
    void cancelAll();

    /**
     * @brief Pausa un job en ejecución.
     * @param jobId ID del job a pausar
     */
    bool pauseJob(const QString &jobId);

    /**
     * @brief Reanuda un job pausado.
     * @param jobId ID del job a reanudar
     */
    bool resumeJob(const QString &jobId);

    /**
     * @brief Obtiene el estado de un job.
     * @param jobId ID del job
     * @return Estado del job o nullo si no existe
     */
    std::optional<JobStatus> getJobStatus(const QString &jobId) const;

    /**
     * @brief Lista todos los jobs con su estado actual.
     */
    QList<JobStatus> getAllJobs() const;

    /**
     * @brief Limpia jobs completados/cancelados/fallidos antiguos.
     * @param olderThan Mantener solo jobs más recientes que esto (en horas)
     */
    void cleanupOldJobs(int olderThanHours = 24);

    /**
     * @brief Número de jobs en ejecución actualmente.
     */
    int activeJobCount() const;

    /**
     * @brief Número total de jobs en cola.
     */
    int queuedJobCount() const;

signals:
    void jobEnqueued(const QString &jobId, JobType type);
    void jobStarted(const QString &jobId);
    void jobProgressUpdated(const QString &jobId, double progress);
    void jobCompleted(const QString &jobId, const QVariant &result);
    void jobFailed(const QString &jobId, const JobError &error);
    void jobCancelled(const QString &jobId);
    void jobPaused(const QString &jobId);
    void jobResumed(const QString &jobId);
    void jobRetrying(const QString &jobId, int attempt);

private slots:
    void processNextJob();
    void onJobFinished(const QString &jobId, const QVariant &result, const JobError &error);

private:
    struct InternalJob {
        JobStatus status;
        QFutureWatcher<QVariant> *watcher = nullptr;
        std::atomic<bool> cancelled{false};
        std::atomic<bool> paused{false};
    };

    void executeJob(InternalJob &job);
    void handleJobFailure(InternalJob &job, const JobError &error);
    void scheduleExecution();

    mutable QMutex m_mutex;
    QQueue<QString> m_jobQueue;
    QMap<QString, InternalJob> m_jobs;
    int m_maxConcurrentJobs;
    std::atomic<int> m_activeCount{0};
    
    friend class JobSystemTest;
};

} // namespace ccos::core
