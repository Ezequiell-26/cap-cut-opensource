#include "core/JobSystem.hpp"

#include <QLoggingCategory>
#include <QMetaObject>
#include <QUuid>
#include <QtConcurrent>

#include <algorithm>
#include <exception>

Q_LOGGING_CATEGORY(ccos_core_job, "ccos.core.job")

namespace ccos::core {

JobSystem::JobSystem(int maxConcurrentJobs, QObject* parent)
    : QObject(parent)
    , m_maxConcurrentJobs(std::max(1, maxConcurrentJobs)) {
    qRegisterMetaType<JobState>("ccos::core::JobState");
    qRegisterMetaType<JobType>("ccos::core::JobType");
    qRegisterMetaType<JobError>("ccos::core::JobError");
    qCInfo(ccos_core_job) << "JobSystem initialized with max concurrent jobs:" << m_maxConcurrentJobs;
}

JobSystem::~JobSystem() {
    cancelAll();
}

QString JobSystem::enqueue(const JobConfig& config) {
    if (!config.executeFn && !config.cooperativeExecuteFn) {
        qCWarning(ccos_core_job) << "Rejected job without execute function";
        return {};
    }

    const QString jobId = config.id.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : config.id;

    const auto job = std::make_shared<InternalJob>();
    job->config = config;
    job->control = std::make_shared<JobControl>();
    job->status.id = jobId;
    job->status.type = config.type;
    job->status.name = config.name.isEmpty() ? QStringLiteral("Job-%1").arg(jobId.left(8)) : config.name;
    job->status.state = JobState::QUEUED;
    job->status.priority = std::clamp(config.priority, 0, 10);
    job->status.createdAt = QDateTime::currentDateTimeUtc();
    job->status.maxRetries = std::max(0, config.maxRetries);
    job->status.cancelable = config.cancelable;
    job->status.userData = config.userData;

    {
        QMutexLocker locker(&m_mutex);
        if (m_jobs.contains(jobId)) {
            qCWarning(ccos_core_job) << "Duplicate job ID rejected:" << jobId;
            return {};
        }

        m_jobs.insert(jobId, job);

        int insertIndex = m_jobQueue.size();
        for (int i = 0; i < m_jobQueue.size(); ++i) {
            const auto existing = m_jobs.value(m_jobQueue.at(i));
            if (existing && existing->status.priority < job->status.priority) {
                insertIndex = i;
                break;
            }
        }
        m_jobQueue.insert(insertIndex, jobId);
    }

    emit jobEnqueued(jobId, config.type);
    scheduleExecution();
    return jobId;
}

bool JobSystem::cancelJob(const QString& jobId) {
    JobPtr job;
    bool cancelledImmediately = false;

    {
        QMutexLocker locker(&m_mutex);
        const auto it = m_jobs.constFind(jobId);
        if (it == m_jobs.cend()) return false;
        job = it.value();

        if (!job->status.cancelable) return false;
        if (job->status.state == JobState::COMPLETED || job->status.state == JobState::FAILED ||
            job->status.state == JobState::CANCELLED) {
            return false;
        }

        job->control->requestCancel();

        if (job->status.state == JobState::QUEUED) {
            job->status.state = JobState::CANCELLED;
            job->status.completedAt = QDateTime::currentDateTimeUtc();
            m_jobQueue.removeAll(jobId);
            cancelledImmediately = true;
        } else if (job->status.state == JobState::RUNNING || job->status.state == JobState::PAUSED) {
            job->status.state = JobState::CANCELLING;
        }
    }

    if (cancelledImmediately) emit jobCancelled(jobId);
    scheduleExecution();
    return true;
}

void JobSystem::cancelAll() {
    QStringList cancelledQueued;
    {
        QMutexLocker locker(&m_mutex);
        for (auto it = m_jobs.cbegin(); it != m_jobs.cend(); ++it) {
            const auto& job = it.value();
            if (!job || !job->status.cancelable) continue;

            if (job->status.state == JobState::QUEUED) {
                job->control->requestCancel();
                job->status.state = JobState::CANCELLED;
                job->status.completedAt = QDateTime::currentDateTimeUtc();
                cancelledQueued.append(job->status.id);
            } else if (job->status.state == JobState::RUNNING || job->status.state == JobState::PAUSED) {
                job->control->requestCancel();
                job->status.state = JobState::CANCELLING;
            }
        }
        m_jobQueue.clear();
    }

    for (const auto& id : cancelledQueued) emit jobCancelled(id);
}

bool JobSystem::pauseJob(const QString& jobId) {
    QMutexLocker locker(&m_mutex);
    const auto it = m_jobs.constFind(jobId);
    if (it == m_jobs.cend()) return false;
    const auto& job = it.value();
    if (!job || job->status.state != JobState::RUNNING || !job->status.cancelable) return false;

    job->control->requestPause();
    job->status.state = JobState::PAUSED;
    locker.unlock();
    emit jobPaused(jobId);
    return true;
}

bool JobSystem::resumeJob(const QString& jobId) {
    QMutexLocker locker(&m_mutex);
    const auto it = m_jobs.constFind(jobId);
    if (it == m_jobs.cend()) return false;
    const auto& job = it.value();
    if (!job || job->status.state != JobState::PAUSED) return false;

    job->control->requestResume();
    job->status.state = JobState::RUNNING;
    locker.unlock();
    emit jobResumed(jobId);
    return true;
}

std::optional<JobStatus> JobSystem::getJobStatus(const QString& jobId) const {
    QMutexLocker locker(&m_mutex);
    const auto it = m_jobs.constFind(jobId);
    return it == m_jobs.cend() ? std::nullopt : std::optional<JobStatus>(it.value()->status);
}

QList<JobStatus> JobSystem::getAllJobs() const {
    QMutexLocker locker(&m_mutex);
    QList<JobStatus> result;
    result.reserve(m_jobs.size());
    for (auto it = m_jobs.cbegin(); it != m_jobs.cend(); ++it) {
        if (it.value()) result.append(it.value()->status);
    }
    return result;
}

void JobSystem::cleanupOldJobs(int olderThanHours) {
    const QDateTime threshold = QDateTime::currentDateTimeUtc().addSecs(-std::max(0, olderThanHours) * 3600);
    QMutexLocker locker(&m_mutex);
    for (auto it = m_jobs.begin(); it != m_jobs.end();) {
        const auto& job = it.value();
        const bool terminal = job && (job->status.state == JobState::COMPLETED ||
                                      job->status.state == JobState::FAILED ||
                                      job->status.state == JobState::CANCELLED);
        if (terminal && job->status.completedAt.isValid() && job->status.completedAt < threshold) {
            it = m_jobs.erase(it);
        } else {
            ++it;
        }
    }
}

int JobSystem::activeJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_activeCount;
}

int JobSystem::queuedJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_jobQueue.size();
}

void JobSystem::processNextJobs() {
    for (;;) {
        JobPtr job;
        {
            QMutexLocker locker(&m_mutex);
            if (m_activeCount >= m_maxConcurrentJobs || m_jobQueue.isEmpty()) return;

            while (!m_jobQueue.isEmpty()) {
                const QString jobId = m_jobQueue.dequeue();
                const auto candidate = m_jobs.value(jobId);
                if (!candidate || candidate->status.state != JobState::QUEUED) continue;
                job = candidate;
                job->status.state = JobState::RUNNING;
                job->status.startedAt = QDateTime::currentDateTimeUtc();
                ++m_activeCount;
                break;
            }
        }

        if (!job) return;
        emit jobStarted(job->status.id);
        executeJob(job);
    }
}

void JobSystem::executeJob(const JobPtr& job) {
    auto* watcher = new QFutureWatcher<ExecutionOutcome>(this);
    job->watcher = watcher;

    const QString jobId = job->status.id;
    const JobControlPtr control = job->control;
    const auto executeFn = job->config.executeFn;
    const auto cooperativeFn = job->config.cooperativeExecuteFn;
    const bool retryExceptions = job->config.retryExceptions;

    connect(watcher, &QFutureWatcher<ExecutionOutcome>::finished, this,
            [this, watcher, jobId]() {
                const ExecutionOutcome outcome = watcher->result();
                watcher->deleteLater();
                onJobFinished(jobId, outcome.result, outcome.error);
            });

    const auto future = QtConcurrent::run([control, executeFn, cooperativeFn, retryExceptions]() -> ExecutionOutcome {
        try {
            if (control->isCancelled()) {
                return {{}, JobError::failed(QStringLiteral("Job cancelled before execution"), -2, false)};
            }

            QVariant result;
            if (cooperativeFn) result = cooperativeFn(control);
            else result = executeFn();

            if (control->isCancelled()) {
                return {std::move(result), JobError::failed(QStringLiteral("Job cancellation requested"), -2, false)};
            }
            return {std::move(result), JobError::success()};
        } catch (const std::exception& exception) {
            return {{}, JobError::failed(QString::fromUtf8(exception.what()), -1, retryExceptions)};
        } catch (...) {
            return {{}, JobError::failed(QStringLiteral("Unknown exception from job"), -1, retryExceptions)};
        }
    });

    watcher->setFuture(future);
}

void JobSystem::onJobFinished(const QString& jobId, const QVariant& result, const JobError& error) {
    JobPtr job;
    bool retrying = false;

    {
        QMutexLocker locker(&m_mutex);
        const auto it = m_jobs.constFind(jobId);
        if (it == m_jobs.cend()) return;
        job = it.value();

        if (m_activeCount > 0) --m_activeCount;
        job->watcher = nullptr;

        if (job->control->isCancelled() || job->status.state == JobState::CANCELLING) {
            job->status.state = JobState::CANCELLED;
            job->status.completedAt = QDateTime::currentDateTimeUtc();
        } else if (error.code == 0 && error.message.isEmpty()) {
            job->status.state = JobState::COMPLETED;
            job->status.completedAt = QDateTime::currentDateTimeUtc();
            job->status.result = result;
        } else {
            handleJobFailure(job, error);
            retrying = job->status.state == JobState::QUEUED;
        }
    }

    if (job->status.state == JobState::COMPLETED) {
        if (job->config.progressCallback) job->config.progressCallback(1.0);
        if (job->config.completionCallback) job->config.completionCallback(JobError::success());
        emit jobCompleted(jobId, result);
    } else if (job->status.state == JobState::CANCELLED) {
        if (job->config.completionCallback) job->config.completionCallback(JobError::failed(QStringLiteral("Job cancelled"), -2, false));
        emit jobCancelled(jobId);
    } else if (!retrying && job->status.state == JobState::FAILED) {
        if (job->config.completionCallback) job->config.completionCallback(job->status.error);
        emit jobFailed(jobId, job->status.error);
    }

    scheduleExecution();
}

void JobSystem::handleJobFailure(const JobPtr& job, const JobError& error) {
    job->status.error = error;

    if (error.retryable && job->status.retryCount < job->status.maxRetries && !job->control->isCancelled()) {
        ++job->status.retryCount;
        job->control->requestResume();
        job->status.state = JobState::QUEUED;
        job->status.priority = std::min(10, job->status.priority + 1);
        m_jobQueue.prepend(job->status.id);
        emit jobRetrying(job->status.id, job->status.retryCount);
        return;
    }

    job->status.state = JobState::FAILED;
    job->status.completedAt = QDateTime::currentDateTimeUtc();
}

void JobSystem::scheduleExecution() {
    QMetaObject::invokeMethod(this, &JobSystem::processNextJobs, Qt::QueuedConnection);
}

} // namespace ccos::core
