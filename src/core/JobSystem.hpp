#pragma once

#include <QObject>
#include <QDateTime>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QMutex>
#include <QPointer>
#include <QQueue>
#include <QSharedPointer>
#include <QString>
#include <QVariant>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>

namespace ccos::core {

enum class JobState {
    QUEUED,
    RUNNING,
    PAUSED,
    CANCELLING,
    CANCELLED,
    FAILED,
    COMPLETED
};

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

struct JobError {
    int code = 0;
    QString message;
    QString details;
    bool recoverable = false;
    bool retryable = false;

    static JobError success() { return {}; }

    static JobError failed(const QString& msg, int errorCode = -1, bool canRetry = false) {
        JobError error;
        error.code = errorCode;
        error.message = msg;
        error.retryable = canRetry;
        return error;
    }
};

/** Cooperative control for jobs that need cancellation or pause support. */
class JobControl final {
public:
    [[nodiscard]] bool isCancelled() const noexcept {
        return m_cancelled.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool isPaused() const noexcept {
        return m_paused.load(std::memory_order_relaxed);
    }

    void requestCancel() noexcept {
        m_cancelled.store(true, std::memory_order_relaxed);
        m_paused.store(false, std::memory_order_relaxed);
    }

    void requestPause() noexcept {
        if (!isCancelled()) m_paused.store(true, std::memory_order_relaxed);
    }

    void requestResume() noexcept {
        m_paused.store(false, std::memory_order_relaxed);
    }

private:
    std::atomic_bool m_cancelled{false};
    std::atomic_bool m_paused{false};
};

using JobControlPtr = std::shared_ptr<JobControl>;

struct JobConfig {
    JobType type = JobType::CUSTOM;
    QString id;
    QString name;
    int priority = 5;
    std::function<QVariant()> executeFn;
    std::function<QVariant(const JobControlPtr&)> cooperativeExecuteFn;
    std::function<void(double)> progressCallback;
    std::function<void(const JobError&)> completionCallback;
    int maxRetries = 1;
    bool cancelable = true;
    QVariant userData;
};

struct JobStatus {
    QString id;
    JobType type = JobType::CUSTOM;
    QString name;
    JobState state = JobState::QUEUED;
    int priority = 5;
    double progress = 0.0;
    QDateTime createdAt;
    QDateTime startedAt;
    QDateTime completedAt;
    JobError error = JobError::success();
    int retryCount = 0;
    int maxRetries = 1;
    bool cancelable = true;
    QVariant result;
    QVariant userData;

    [[nodiscard]] QString stateToString() const {
        switch (state) {
        case JobState::QUEUED: return QStringLiteral("QUEUED");
        case JobState::RUNNING: return QStringLiteral("RUNNING");
        case JobState::PAUSED: return QStringLiteral("PAUSED");
        case JobState::CANCELLING: return QStringLiteral("CANCELLING");
        case JobState::CANCELLED: return QStringLiteral("CANCELLED");
        case JobState::FAILED: return QStringLiteral("FAILED");
        case JobState::COMPLETED: return QStringLiteral("COMPLETED");
        }
        return QStringLiteral("UNKNOWN");
    }

    [[nodiscard]] QString typeToString() const {
        switch (type) {
        case JobType::IMPORT_MEDIA: return QStringLiteral("IMPORT_MEDIA");
        case JobType::PROBE_MEDIA: return QStringLiteral("PROBE_MEDIA");
        case JobType::GENERATE_THUMBNAIL: return QStringLiteral("GENERATE_THUMBNAIL");
        case JobType::GENERATE_WAVEFORM: return QStringLiteral("GENERATE_WAVEFORM");
        case JobType::GENERATE_PROXY: return QStringLiteral("GENERATE_PROXY");
        case JobType::RENDER_PREVIEW: return QStringLiteral("RENDER_PREVIEW");
        case JobType::EXPORT_VIDEO: return QStringLiteral("EXPORT_VIDEO");
        case JobType::TRANSCRIBE_AUDIO: return QStringLiteral("TRANSCRIBE_AUDIO");
        case JobType::AI_ANALYSIS: return QStringLiteral("AI_ANALYSIS");
        case JobType::CACHE_OPERATION: return QStringLiteral("CACHE_OPERATION");
        case JobType::CUSTOM: return QStringLiteral("CUSTOM");
        }
        return QStringLiteral("UNKNOWN");
    }
};

class JobSystem final : public QObject {
    Q_OBJECT

public:
    explicit JobSystem(int maxConcurrentJobs = 4, QObject* parent = nullptr);
    ~JobSystem() override;

    [[nodiscard]] QString enqueue(const JobConfig& config);
    bool cancelJob(const QString& jobId);
    void cancelAll();
    bool pauseJob(const QString& jobId);
    bool resumeJob(const QString& jobId);

    [[nodiscard]] std::optional<JobStatus> getJobStatus(const QString& jobId) const;
    [[nodiscard]] QList<JobStatus> getAllJobs() const;

    void cleanupOldJobs(int olderThanHours = 24);

    [[nodiscard]] int activeJobCount() const;
    [[nodiscard]] int queuedJobCount() const;

signals:
    void jobEnqueued(const QString& jobId, JobType type);
    void jobStarted(const QString& jobId);
    void jobProgressUpdated(const QString& jobId, double progress);
    void jobCompleted(const QString& jobId, const QVariant& result);
    void jobFailed(const QString& jobId, const JobError& error);
    void jobCancelled(const QString& jobId);
    void jobPaused(const QString& jobId);
    void jobResumed(const QString& jobId);
    void jobRetrying(const QString& jobId, int attempt);

private slots:
    void processNextJobs();
    void onJobFinished(const QString& jobId, const QVariant& result, const JobError& error);

private:
    struct ExecutionOutcome {
        QVariant result;
        JobError error;
    };

    struct InternalJob {
        JobConfig config;
        JobStatus status;
        JobControlPtr control;
        QPointer<QFutureWatcher<ExecutionOutcome>> watcher;
    };

    using JobPtr = std::shared_ptr<InternalJob>;

    void executeJob(const JobPtr& job);
    void handleJobFailure(const JobPtr& job, const JobError& error);
    void scheduleExecution();

    mutable QMutex m_mutex;
    QQueue<QString> m_jobQueue;
    QHash<QString, JobPtr> m_jobs;
    int m_maxConcurrentJobs = 1;
    int m_activeCount = 0;
};

} // namespace ccos::core

Q_DECLARE_METATYPE(ccos::core::JobState)
Q_DECLARE_METATYPE(ccos::core::JobType)
Q_DECLARE_METATYPE(ccos::core::JobError)
