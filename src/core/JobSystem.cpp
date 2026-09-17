#include "core/JobSystem.hpp"
#include <QtConcurrent>
#include <QCoreApplication>
#include <QThread>

Q_LOGGING_CATEGORY(ccos_core_job, "ccos.core.job")

namespace ccos::core {

JobSystem::JobSystem(int maxConcurrentJobs, QObject *parent)
    : QObject(parent)
    , m_maxConcurrentJobs(maxConcurrentJobs)
{
    qCInfo(ccos_core_job) << "JobSystem initialized with max concurrent jobs:" << maxConcurrentJobs;
}

JobSystem::~JobSystem() {
    cancelAll();
}

QString JobSystem::enqueue(const JobConfig &config) {
    if (!config.executeFn) {
        qCWarning(ccos_core_job) << "Attempted to enqueue job without execute function";
        return {};
    }

    QString jobId = config.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : config.id;

    InternalJob internalJob;
    internalJob.status.id = jobId;
    internalJob.status.type = config.type;
    internalJob.status.name = config.name.isEmpty() ? QString("Job-%1").arg(jobId.left(8)) : config.name;
    internalJob.status.state = JobState::QUEUED;
    internalJob.status.priority = config.priority;
    internalJob.status.createdAt = QDateTime::currentDateTime();
    internalJob.status.maxRetries = config.maxRetries;
    internalJob.status.cancelable = config.cancelable;
    internalJob.status.userData = config.userData;

    {
        QMutexLocker locker(&m_mutex);
        
        // Verificar duplicados
        if (m_jobs.contains(jobId)) {
            qCWarning(ccos_core_job) << "Job with ID already exists:" << jobId;
            return {};
        }

        m_jobs[jobId] = std::move(internalJob);
        
        // Insertar en cola según prioridad (simple insertion sort)
        bool inserted = false;
        for (int i = 0; i < m_jobQueue.size(); ++i) {
            const auto &existingId = m_jobQueue[i];
            auto it = m_jobs.find(existingId);
            if (it != m_jobs.end() && it->status.priority < config.priority) {
                m_jobQueue.insert(i, jobId);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            m_jobQueue.enqueue(jobId);
        }
    }

    emit jobEnqueued(jobId, config.type);
    qCInfo(ccos_core_job) << "Job enqueued:" << jobId << "type:" << static_cast<int>(config.type) 
                          << "priority:" << config.priority;

    // Intentar procesar inmediatamente si hay capacidad
    scheduleExecution();

    return jobId;
}

bool JobSystem::cancelJob(const QString &jobId) {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        qCWarning(ccos_core_job) << "Cannot cancel non-existent job:" << jobId;
        return false;
    }

    auto &job = *it;
    
    if (!job.status.cancelable) {
        qCWarning(ccos_core_job) << "Job is not cancelable:" << jobId;
        return false;
    }

    if (job.status.state == JobState::COMPLETED || 
        job.status.state == JobState::CANCELLED) {
        qCWarning(ccos_core_job) << "Cannot cancel job in state:" << job.status.stateToString();
        return false;
    }

    if (job.status.state == JobState::RUNNING) {
        job.status.state = JobState::CANCELLING;
        job.cancelled = true;
        qCInfo(ccos_core_job) << "Job cancellation requested:" << jobId;
    } else if (job.status.state == JobState::QUEUED) {
        job.status.state = JobState::CANCELLED;
        job.status.completedAt = QDateTime::currentDateTime();
        // Remover de la cola
        m_jobQueue.removeAll(jobId);
        emit jobCancelled(jobId);
        qCInfo(ccos_core_job) << "Job cancelled before execution:" << jobId;
    }

    return true;
}

void JobSystem::cancelAll() {
    QMutexLocker locker(&m_mutex);
    
    qCInfo(ccos_core_job) << "Cancelling all jobs...";
    
    for (auto &pair : m_jobs.asKeyValueRange()) {
        auto &job = pair.value;
        
        if (!job.status.cancelable) continue;
        
        if (job.status.state == JobState::RUNNING) {
            job.status.state = JobState::CANCELLING;
            job.cancelled = true;
        } else if (job.status.state == JobState::QUEUED) {
            job.status.state = JobState::CANCELLED;
            job.status.completedAt = QDateTime::currentDateTime();
            emit jobCancelled(job.status.id);
        }
    }
    
    m_jobQueue.clear();
}

bool JobSystem::pauseJob(const QString &jobId) {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return false;
    }

    auto &job = *it;
    
    if (job.status.state != JobState::RUNNING) {
        qCWarning(ccos_core_job) << "Can only pause running jobs, current state:" << job.status.stateToString();
        return false;
    }

    job.status.state = JobState::PAUSED;
    job.paused = true;
    emit jobPaused(jobId);
    qCInfo(ccos_core_job) << "Job paused:" << jobId;
    
    return true;
}

bool JobSystem::resumeJob(const QString &jobId) {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return false;
    }

    auto &job = *it;
    
    if (job.status.state != JobState::PAUSED) {
        qCWarning(ccos_core_job) << "Can only resume paused jobs, current state:" << job.status.stateToString();
        return false;
    }

    job.status.state = JobState::RUNNING;
    job.paused = false;
    emit jobResumed(jobId);
    qCInfo(ccos_core_job) << "Job resumed:" << jobId;
    
    return true;
}

std::optional<JobStatus> JobSystem::getJobStatus(const QString &jobId) const {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return std::nullopt;
    }

    return it->status;
}

QList<JobStatus> JobSystem::getAllJobs() const {
    QMutexLocker locker(&m_mutex);
    
    QList<JobStatus> result;
    result.reserve(m_jobs.size());
    
    for (const auto &pair : m_jobs.asKeyValueRange()) {
        result.append(pair.value.status);
    }
    
    return result;
}

void JobSystem::cleanupOldJobs(int olderThanHours) {
    QMutexLocker locker(&m_mutex);
    
    QDateTime threshold = QDateTime::currentDateTime().addSecs(-olderThanHours * 3600);
    
    QStringList toRemove;
    
    for (const auto &pair : m_jobs.asKeyValueRange()) {
        const auto &job = pair.value;
        
        // Solo limpiar jobs terminados
        if (job.status.state != JobState::COMPLETED &&
            job.status.state != JobState::CANCELLED &&
            job.status.state != JobState::FAILED) {
            continue;
        }
        
        if (job.status.completedAt < threshold) {
            toRemove.append(pair.key);
        }
    }
    
    for (const QString &jobId : toRemove) {
        m_jobs.remove(jobId);
    }
    
    if (!toRemove.isEmpty()) {
        qCInfo(ccos_core_job) << "Cleaned up" << toRemove.size() << "old jobs";
    }
}

int JobSystem::activeJobCount() const {
    QMutexLocker locker(&m_mutex);
    
    int count = 0;
    for (const auto &pair : m_jobs.asKeyValueRange()) {
        if (pair.value.status.state == JobState::RUNNING) {
            ++count;
        }
    }
    return count;
}

int JobSystem::queuedJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_jobQueue.size();
}

void JobSystem::processNextJob() {
    QMutexLocker locker(&m_mutex);
    
    // Verificar capacidad
    if (m_activeCount >= m_maxConcurrentJobs) {
        return;
    }

    // Buscar siguiente job no pausado
    while (!m_jobQueue.isEmpty()) {
        QString jobId = m_jobQueue.head();
        auto it = m_jobs.find(jobId);
        
        if (it == m_jobs.end()) {
            m_jobQueue.dequeue();
            continue;
        }

        auto &job = *it;
        
        // Saltar jobs que no están en estado QUEUED
        if (job.status.state != JobState::QUEUED) {
            m_jobQueue.dequeue();
            continue;
        }

        // Mover a RUNNING
        job.status.state = JobState::RUNNING;
        job.status.startedAt = QDateTime::currentDateTime();
        
        m_jobQueue.dequeue();
        ++m_activeCount;
        
        qCInfo(ccos_core_job) << "Starting job:" << jobId << "active count:" << m_activeCount.load();
        emit jobStarted(jobId);
        
        // Ejecutar en thread pool
        executeJob(job);
        return;
    }
}

void JobSystem::onJobFinished(const QString &jobId, const QVariant &result, const JobError &error) {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        qCWarning(ccos_core_job) << "Job finished but not found:" << jobId;
        return;
    }

    auto &job = *it;
    
    // Limpiar watcher
    if (job.watcher) {
        job.watcher->deleteLater();
        job.watcher = nullptr;
    }

    --m_activeCount;

    // Verificar si fue cancelado
    if (job.status.state == JobState::CANCELLING || job.cancelled) {
        job.status.state = JobState::CANCELLED;
        job.status.completedAt = QDateTime::currentDateTime();
        emit jobCancelled(jobId);
        qCInfo(ccos_core_job) << "Job cancelled successfully:" << jobId;
        return;
    }

    // Manejar error o éxito
    if (error.code != 0 || !error.message.isEmpty()) {
        handleJobFailure(job, error);
    } else {
        job.status.state = JobState::COMPLETED;
        job.status.completedAt = QDateTime::currentDateTime();
        job.status.result = result;
        emit jobCompleted(jobId, result);
        qCInfo(ccos_core_job) << "Job completed successfully:" << jobId;
    }

    // Procesar siguiente job
    scheduleExecution();
}

void JobSystem::executeJob(InternalJob &job) {
    const QString jobId = job.status.id;
    auto executeFn = job.status.userData.value<std::function<QVariant()>>();
    
    // Crear watcher para monitorear el futuro
    auto *watcher = new QFutureWatcher<QVariant>();
    job.watcher = watcher;

    connect(watcher, &QFutureWatcher<QVariant>::finished, this, [this, jobId, watcher]() {
        if (watcher->isCanceled()) {
            onJobFinished(jobId, {}, JobError::failed("Job was canceled", -1, false));
            return;
        }
        
        if (watcher->isFinished()) {
            try {
                QVariant result = watcher->result();
                onJobFinished(jobId, result, JobError::success());
            } catch (const std::exception &e) {
                onJobFinished(jobId, {}, JobError::failed(QString::fromUtf8(e.what()), -1, false));
            } catch (...) {
                onJobFinished(jobId, {}, JobError::failed("Unknown exception", -1, false));
            }
        }
    });

    // Ejecutar en QtConcurrent
    auto future = QtConcurrent::run([this, jobId, &job, executeFn]() {
        // Verificar cancelación periódicamente
        auto checkCancellation = [&]() -> bool {
            QMutexLocker locker(&m_mutex);
            auto it = m_jobs.find(jobId);
            if (it == m_jobs.end() || it->cancelled || it->status.state == JobState::CANCELLING) {
                return true;
            }
            
            // Verificar pausa
            if (it->paused) {
                // Esperar mientras está pausado
                QThread::msleep(100);
                return checkCancellation();
            }
            
            return false;
        };

        try {
            if (executeFn) {
                return executeFn();
            }
            return QVariant{};
        } catch (...) {
            throw;
        }
    });

    watcher->setFuture(future);
}

void JobSystem::handleJobFailure(InternalJob &job, const JobError &error) {
    const QString jobId = job.status.id;
    
    job.status.error = error;
    
    // Verificar reintentos
    if (job.status.retryCount < job.status.maxRetries && error.retryable) {
        job.status.retryCount++;
        job.status.state = JobState::QUEUED;
        
        // Re-encolar con prioridad ligeramente mayor
        job.status.priority = qMin(10, job.status.priority + 1);
        m_jobQueue.enqueue(jobId);
        
        emit jobRetrying(jobId, job.status.retryCount);
        qCInfo(ccos_core_job) << "Job failed, retrying (" << job.status.retryCount 
                              << "/" << job.status.maxRetries << "):" << jobId;
        
        scheduleExecution();
    } else {
        job.status.state = JobState::FAILED;
        job.status.completedAt = QDateTime::currentDateTime();
        emit jobFailed(jobId, error);
        qCWarning(ccos_core_job) << "Job failed permanently:" << jobId 
                                 << "error:" << error.message;
    }
}

void JobSystem::scheduleExecution() {
    // Usar invoke para asegurar ejecución en el thread correcto
    QMetaObject::invokeMethod(this, &JobSystem::processNextJob, Qt::QueuedConnection);
}

} // namespace ccos::core
