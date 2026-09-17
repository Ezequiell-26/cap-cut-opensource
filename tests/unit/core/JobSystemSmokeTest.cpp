#include <gtest/gtest.h>

#include "core/JobSystem.hpp"

#include <QTest>
#include <atomic>
#include <chrono>
#include <thread>

using namespace ccos::core;

TEST(JobSystemSmokeTest, ExecutesAndReportsCompletion) {
    JobSystem jobs(2);
    JobConfig config;
    config.name = QStringLiteral("smoke");
    config.executeFn = [] { return QVariant(42); };

    const QString id = jobs.enqueue(config);
    ASSERT_FALSE(id.isEmpty());

    QTest::qWait(100);
    const auto status = jobs.getJobStatus(id);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::COMPLETED);
    EXPECT_EQ(status->result.toInt(), 42);
}

TEST(JobSystemSmokeTest, CancellationIsObservable) {
    JobSystem jobs(1);
    JobConfig config;
    config.cooperativeExecuteFn = [](const JobControlPtr& control) {
        for (int i = 0; i < 100 && !control->isCancelled(); ++i) {
            QTest::qWait(10);
        }
        return QVariant();
    };

    const QString id = jobs.enqueue(config);
    ASSERT_FALSE(id.isEmpty());
    QTest::qWait(50);
    ASSERT_TRUE(jobs.cancelJob(id));
    QTest::qWait(100);

    const auto status = jobs.getJobStatus(id);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::CANCELLED);
}

TEST(JobSystemSmokeTest, ExplicitRetryPolicyWorks) {
    JobSystem jobs(1);
    std::atomic<int> attempts{0};

    JobConfig config;
    config.maxRetries = 2;
    config.retryExceptions = true;
    config.executeFn = [&attempts] {
        const int attempt = ++attempts;
        if (attempt < 3) throw std::runtime_error("transient failure");
        return QVariant(7);
    };

    const QString id = jobs.enqueue(config);
    ASSERT_FALSE(id.isEmpty());
    QTest::qWait(300);

    const auto status = jobs.getJobStatus(id);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->state, JobState::COMPLETED);
    EXPECT_EQ(status->retryCount, 2);
    EXPECT_EQ(attempts.load(), 3);
}

TEST(JobSystemSmokeTest, ConcurrencyLimitIsRespected) {
    JobSystem jobs(2);
    std::atomic<int> current{0};
    std::atomic<int> maximum{0};

    for (int i = 0; i < 8; ++i) {
        JobConfig config;
        config.executeFn = [&current, &maximum] {
            const int value = ++current;
            int observed = maximum.load();
            while (value > observed && !maximum.compare_exchange_weak(observed, value)) {
            }
            QTest::qWait(50);
            --current;
            return QVariant();
        };
        ASSERT_FALSE(jobs.enqueue(config).isEmpty());
    }

    QTest::qWait(1000);
    EXPECT_LE(maximum.load(), 2);
    EXPECT_EQ(jobs.activeJobCount(), 0);
    EXPECT_EQ(jobs.queuedJobCount(), 0);
}
