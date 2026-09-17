#include <gtest/gtest.h>
#include "core/JobSystem.hpp"
#include <QTest>
#include <QSignalSpy>
#include <chrono>
#include <thread>

using namespace ccos::core;

class JobSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        jobSystem = std::make_unique<JobSystem>(2); // Max 2 concurrent jobs
    }

    void TearDown() override {
        jobSystem->cancelAll();
        jobSystem.reset();
    }

    std::unique_ptr<JobSystem> jobSystem;
};

/**
 * @brief Test: Creación básica del JobSystem
 */
TEST_F(JobSystemTest, Constructor) {
    EXPECT_NE(jobSystem, nullptr);
    EXPECT_EQ(jobSystem->activeJobCount(), 0);
    EXPECT_EQ(jobSystem->queuedJobCount(), 0);
}

/**
 * @brief Test: Enqueue de job simple
 */
TEST_F(JobSystemTest, EnqueueSimpleJob) {
    JobConfig config;
    config.type = JobType::CUSTOM;
    config.name = "Test Job";
    config.executeFn = []() -> QVariant {
        return QVariant(42);
    };

    QString jobId = jobSystem->enqueue(config);
    
    EXPECT_FALSE(jobId.isEmpty());
    
    // Esperar a que complete
    QTest::qWait(500);
    
    auto status = jobSystem->getJobStatus(jobId);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::COMPLETED);
    EXPECT_EQ(status->name, "Test Job");
}

/**
 * @brief Test: Cancelación de job en cola
 */
TEST_F(JobSystemTest, CancelQueuedJob) {
    JobConfig config;
    config.type = JobType::CUSTOM;
    config.executeFn = []() -> QVariant {
        QTest::qWait(1000);
        return QVariant();
    };

    QString jobId = jobSystem->enqueue(config);
    EXPECT_FALSE(jobId.isEmpty());
    
    // Cancelar inmediatamente (debería estar en cola o empezando)
    bool cancelled = jobSystem->cancelJob(jobId);
    
    // Esperar un poco para ver el resultado
    QTest::qWait(200);
    
    auto status = jobSystem->getJobStatus(jobId);
    ASSERT_TRUE(status.has_value());
    EXPECT_TRUE(status->state == JobState::CANCELLED || 
                status->state == JobState::CANCELLING ||
                status->state == JobState::COMPLETED); // Si ya terminó antes de cancelar
}

/**
 * @brief Test: Múltiples jobs con prioridad
 */
TEST_F(JobSystemTest, MultipleJobsWithPriority) {
    QStringList executionOrder;
    QMutex mutex;

    auto createJob = [&](const QString &name, int priority) {
        JobConfig config;
        config.type = JobType::CUSTOM;
        config.name = name;
        config.priority = priority;
        config.executeFn = [&, name]() -> QVariant {
            QMutexLocker locker(&mutex);
            executionOrder.append(name);
            QTest::qWait(100);
            return QVariant();
        };
        return config;
    };

    // Encolar jobs con diferentes prioridades
    QString id1 = jobSystem->enqueue(createJob("Low", 1));
    QString id2 = jobSystem->enqueue(createJob("High", 10));
    QString id3 = jobSystem->enqueue(createJob("Medium", 5));

    EXPECT_FALSE(id1.isEmpty());
    EXPECT_FALSE(id2.isEmpty());
    EXPECT_FALSE(id3.isEmpty());

    // Esperar a que todos completen
    QTest::qWait(2000);

    // Verificar que el de alta prioridad se ejecutó antes
    QMutexLocker locker(&mutex);
    if (!executionOrder.isEmpty()) {
        EXPECT_EQ(executionOrder.first(), "High") << "Expected high priority job to execute first, got: " 
                                                   << executionOrder.first().toStdString();
    }
}

/**
 * @brief Test: Job que falla sin reintentos
 */
TEST_F(JobSystemTest, JobFailureNoRetries) {
    JobConfig config;
    config.type = JobType::CUSTOM;
    config.maxRetries = 0;
    config.executeFn = []() -> QVariant {
        throw std::runtime_error("Intentional failure");
    };

    QString jobId = jobSystem->enqueue(config);
    
    // Esperar a que falle
    QTest::qWait(500);
    
    auto status = jobSystem->getJobStatus(jobId);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::FAILED);
    EXPECT_FALSE(status->error.message.isEmpty());
}

/**
 * @brief Test: Job con reintentos
 */
TEST_F(JobSystemTest, JobWithRetries) {
    std::atomic<int> attemptCount{0};
    
    JobConfig config;
    config.type = JobType::CUSTOM;
    config.maxRetries = 2;
    config.executeFn = [&]() -> QVariant {
        int attempt = ++attemptCount;
        if (attempt < 3) {
            throw std::runtime_error(("Failure on attempt " + std::to_string(attempt)).c_str());
        }
        return QVariant(42);
    };

    QString jobId = jobSystem->enqueue(config);
    
    // Esperar a que complete con retries
    QTest::qWait(1000);
    
    auto status = jobSystem->getJobStatus(jobId);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::COMPLETED);
    EXPECT_GE(status->retryCount, 2);
}

/**
 * @brief Test: Pausa y reanudación
 */
TEST_F(JobSystemTest, PauseAndResume) {
    std::atomic<bool> paused{false};
    std::atomic<int> progress{0};
    
    JobConfig config;
    config.type = JobType::CUSTOM;
    config.executeFn = [&]() -> QVariant {
        for (int i = 0; i < 10; ++i) {
            if (paused) {
                QTest::qWait(500); // Esperar mientras está pausado
            }
            progress++;
            QTest::qWait(50);
        }
        return QVariant();
    };

    QString jobId = jobSystem->enqueue(config);
    
    // Esperar un poco y pausar
    QTest::qWait(200);
    bool pauseResult = jobSystem->pauseJob(jobId);
    EXPECT_TRUE(pauseResult);
    
    auto status = jobSystem->getJobStatus(jobId);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::PAUSED);
    
    // Reanudar
    bool resumeResult = jobSystem->resumeJob(jobId);
    EXPECT_TRUE(resumeResult);
    
    // Esperar a que complete
    QTest::qWait(1000);
    
    status = jobSystem->getJobStatus(jobId);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::COMPLETED);
}

/**
 * @brief Test: Cancelar todos los jobs
 */
TEST_F(JobSystemTest, CancelAllJobs) {
    const int jobCount = 5;
    QStringList jobIds;
    
    for (int i = 0; i < jobCount; ++i) {
        JobConfig config;
        config.type = JobType::CUSTOM;
        config.executeFn = []() -> QVariant {
            QTest::qWait(1000);
            return QVariant();
        };
        QString id = jobSystem->enqueue(config);
        EXPECT_FALSE(id.isEmpty());
        jobIds.append(id);
    }
    
    // Cancelar todos
    jobSystem->cancelAll();
    
    // Esperar un poco
    QTest::qWait(500);
    
    // Verificar que todos están cancelados o completados
    int cancelledOrCompleted = 0;
    for (const QString &id : jobIds) {
        auto status = jobSystem->getJobStatus(id);
        if (status.has_value()) {
            if (status->state == JobState::CANCELLED || 
                status->state == JobState::COMPLETED) {
                cancelledOrCompleted++;
            }
        }
    }
    
    EXPECT_EQ(cancelledOrCompleted, jobCount);
}

/**
 * @brief Test: Límite de jobs concurrentes
 */
TEST_F(JobSystemTest, ConcurrentJobLimit) {
    std::atomic<int> maxConcurrent{0};
    std::atomic<int> currentConcurrent{0};
    
    const int jobCount = 10;
    
    for (int i = 0; i < jobCount; ++i) {
        JobConfig config;
        config.type = JobType::CUSTOM;
        config.executeFn = [&]() -> QVariant {
            int current = ++currentConcurrent;
            int max = maxConcurrent.load();
            while (current > max) {
                if (maxConcurrent.compare_exchange_weak(max, current)) {
                    break;
                }
            }
            QTest::qWait(200);
            --currentConcurrent;
            return QVariant();
        };
        jobSystem->enqueue(config);
    }
    
    // Esperar a que todos completen
    QTest::qWait(3000);
    
    // Verificar que no excedió el límite (2 jobs concurrentes máx)
    EXPECT_LE(maxConcurrent.load(), 2) << "Max concurrent jobs exceeded limit: " 
                                        << maxConcurrent.load();
}

/**
 * @brief Test: Limpieza de jobs antiguos
 */
TEST_F(JobSystemTest, CleanupOldJobs) {
    // Crear jobs que completen rápidamente
    for (int i = 0; i < 5; ++i) {
        JobConfig config;
        config.type = JobType::CUSTOM;
        config.executeFn = []() -> QVariant { return QVariant(); };
        jobSystem->enqueue(config);
    }
    
    QTest::qWait(500);
    
    int totalJobs = jobSystem->getAllJobs().size();
    EXPECT_EQ(totalJobs, 5);
    
    // Limpiar jobs más antiguos de 0 horas (todos)
    jobSystem->cleanupOldJobs(0);
    
    int remainingJobs = jobSystem->getAllJobs().size();
    EXPECT_LT(remainingJobs, totalJobs);
}

/**
 * @brief Test: Job con ID personalizado
 */
TEST_F(JobSystemTest, CustomJobId) {
    JobConfig config;
    config.id = "my-custom-job-id-123";
    config.type = JobType::IMPORT_MEDIA;
    config.executeFn = []() -> QVariant { return QVariant(42); };

    QString jobId = jobSystem->enqueue(config);
    
    EXPECT_EQ(jobId, "my-custom-job-id-123");
    
    QTest::qWait(200);
    
    auto status = jobSystem->getJobStatus(jobId);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->type, JobType::IMPORT_MEDIA);
}

/**
 * @brief Test: Signals emitidos correctamente
 */
TEST_F(JobSystemTest, SignalsEmitted) {
    QSignalSpy spyEnqueued(jobSystem.get(), SIGNAL(jobEnqueued(QString, JobType)));
    QSignalSpy spyStarted(jobSystem.get(), SIGNAL(jobStarted(QString)));
    QSignalSpy spyCompleted(jobSystem.get(), SIGNAL(jobCompleted(QString, QVariant)));

    JobConfig config;
    config.type = JobType::PROBE_MEDIA;
    config.executeFn = []() -> QVariant { return QVariant(); };

    QString jobId = jobSystem->enqueue(config);
    
    // Esperar a que complete
    QTest::qWait(500);
    
    EXPECT_GE(spyEnqueued.count(), 1);
    EXPECT_GE(spyStarted.count(), 1);
    EXPECT_GE(spyCompleted.count(), 1);
}

/**
 * @brief Test: Job no cancelable
 */
TEST_F(JobSystemTest, NonCancelableJob) {
    JobConfig config;
    config.cancelable = false;
    config.executeFn = []() -> QVariant {
        QTest::qWait(500);
        return QVariant();
    };

    QString jobId = jobSystem->enqueue(config);
    
    // Intentar cancelar
    bool result = jobSystem->cancelJob(jobId);
    
    EXPECT_FALSE(result);
}

/**
 * @brief Test: Obtener todos los jobs
 */
TEST_F(JobSystemTest, GetAllJobs) {
    const int jobCount = 3;
    
    for (int i = 0; i < jobCount; ++i) {
        JobConfig config;
        config.type = static_cast<JobType>(i % 11);
        config.executeFn = []() -> QVariant { return QVariant(); };
        jobSystem->enqueue(config);
    }
    
    QTest::qWait(500);
    
    QList<JobStatus> allJobs = jobSystem->getAllJobs();
    
    EXPECT_EQ(allJobs.size(), jobCount);
    
    // Verificar que cada job tiene ID único
    QSet<QString> ids;
    for (const auto &job : allJobs) {
        EXPECT_FALSE(job.id.isEmpty());
        EXPECT_FALSE(ids.contains(job.id));
        ids.insert(job.id);
    }
}

/**
 * @brief Test: Job state to string conversion
 */
TEST_F(JobSystemTest, StateToString) {
    JobStatus status;
    
    status.state = JobState::QUEUED;
    EXPECT_EQ(status.stateToString(), "QUEUED");
    
    status.state = JobState::RUNNING;
    EXPECT_EQ(status.stateToString(), "RUNNING");
    
    status.state = JobState::COMPLETED;
    EXPECT_EQ(status.stateToString(), "COMPLETED");
    
    status.state = JobState::FAILED;
    EXPECT_EQ(status.stateToString(), "FAILED");
    
    status.state = JobState::CANCELLED;
    EXPECT_EQ(status.stateToString(), "CANCELLED");
}

/**
 * @brief Test: Job type to string conversion
 */
TEST_F(JobSystemTest, TypeToString) {
    JobStatus status;
    
    status.type = JobType::IMPORT_MEDIA;
    EXPECT_EQ(status.typeToString(), "IMPORT_MEDIA");
    
    status.type = JobType::EXPORT_VIDEO;
    EXPECT_EQ(status.typeToString(), "EXPORT_VIDEO");
    
    status.type = JobType::TRANSCRIBE_AUDIO;
    EXPECT_EQ(status.typeToString(), "TRANSCRIBE_AUDIO");
}
