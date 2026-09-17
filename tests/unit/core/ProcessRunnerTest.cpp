#include <gtest/gtest.h>

#include "core/ProcessRunner.hpp"

#include <QFuture>
#include <QProcessEnvironment>
#include <QStringList>
#include <thread>

using namespace ccos::core;

TEST(ProcessRunnerTest, ValidatesExecutableOnPath) {
    EXPECT_TRUE(ProcessRunner::validateExecutable(QStringLiteral("cmake")));
    EXPECT_FALSE(ProcessRunner::validateExecutable(QStringLiteral("ccos-definitely-does-not-exist")));
}

TEST(ProcessRunnerTest, SynchronousExecutionUsesArgumentList) {
    ProcessRunner runner;
    ProcessConfig config;
    config.executable = QStringLiteral("cmake");
    config.arguments = {QStringLiteral("-E"), QStringLiteral("echo"), QStringLiteral("hello ccos")};
    config.timeout = std::chrono::seconds(5);

    const ProcessResult result = runner.executeSync(config);

    ASSERT_TRUE(result.started);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_NE(QString::fromUtf8(result.standardOutput).indexOf(QStringLiteral("hello ccos")), -1);
}

TEST(ProcessRunnerTest, TimeoutTerminatesLongRunningProcess) {
    ProcessRunner runner;
    ProcessConfig config;
    config.executable = QStringLiteral("cmake");
    config.arguments = {QStringLiteral("-E"), QStringLiteral("sleep"), QStringLiteral("2")};
    config.timeout = std::chrono::milliseconds(100);
    config.startupTimeout = std::chrono::seconds(5);

    const ProcessResult result = runner.executeSync(config);

    ASSERT_TRUE(result.started);
    EXPECT_TRUE(result.timedOut);
    EXPECT_FALSE(result.isSuccess());
}

TEST(ProcessRunnerTest, AsyncExecutionCompletesItsFuture) {
    ProcessRunner runner;
    ProcessConfig config;
    config.executable = QStringLiteral("cmake");
    config.arguments = {QStringLiteral("-E"), QStringLiteral("echo"), QStringLiteral("async")};
    config.timeout = std::chrono::seconds(5);

    QFuture<ProcessResult> future = runner.execute(config);
    future.waitForFinished();
    const ProcessResult result = future.result();

    EXPECT_TRUE(result.isSuccess());
    EXPECT_NE(QString::fromUtf8(result.standardOutput).indexOf(QStringLiteral("async")), -1);
}

TEST(ProcessRunnerTest, CancellationStopsActiveExecution) {
    ProcessRunner runner;
    ProcessConfig config;
    config.executable = QStringLiteral("cmake");
    config.arguments = {QStringLiteral("-E"), QStringLiteral("sleep"), QStringLiteral("2")};
    config.timeout = std::chrono::seconds(5);

    QFuture<ProcessResult> future = runner.execute(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    runner.cancelAll();
    future.waitForFinished();

    const ProcessResult result = future.result();
    EXPECT_TRUE(result.cancelled);
    EXPECT_FALSE(result.isSuccess());
}

TEST(ProcessRunnerTest, OutputIsBounded) {
    ProcessRunner runner;
    ProcessConfig config;
    config.executable = QStringLiteral("cmake");
    config.arguments = {QStringLiteral("-E"), QStringLiteral("echo"), QString(5000, QLatin1Char('x'))};
    config.maxOutputSize = 64;
    config.timeout = std::chrono::seconds(5);

    const ProcessResult result = runner.executeSync(config);

    EXPECT_TRUE(result.outputTruncated);
    EXPECT_LE(result.standardOutput.size(), config.maxOutputSize);
}

TEST(ProcessRunnerTest, SanitizedEnvironmentRemovesCredentialLikeVariables) {
    QProcessEnvironment environment;
    environment.insert(QStringLiteral("PATH"), QStringLiteral("/safe/path"));
    environment.insert(QStringLiteral("OPENAI_API_KEY"), QStringLiteral("secret"));
    environment.insert(QStringLiteral("MY_SESSION_TOKEN"), QStringLiteral("secret"));
    environment.insert(QStringLiteral("BUILD_MODE"), QStringLiteral("release"));

    const QProcessEnvironment sanitized = ProcessRunner::sanitizedEnvironment(environment);

    EXPECT_EQ(sanitized.value(QStringLiteral("PATH")), QStringLiteral("/safe/path"));
    EXPECT_EQ(sanitized.value(QStringLiteral("BUILD_MODE")), QStringLiteral("release"));
    EXPECT_FALSE(sanitized.contains(QStringLiteral("OPENAI_API_KEY")));
    EXPECT_FALSE(sanitized.contains(QStringLiteral("MY_SESSION_TOKEN")));
}

TEST(ProcessRunnerTest, SanitizedEnvironmentPreservesNonCredentialConfiguration) {
    QProcessEnvironment environment;
    environment.insert(QStringLiteral("FFMPEG_FORCE_NOCOLOR"), QStringLiteral("1"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C.UTF-8"));

    const QProcessEnvironment sanitized = ProcessRunner::sanitizedEnvironment(environment);

    EXPECT_EQ(sanitized.value(QStringLiteral("FFMPEG_FORCE_NOCOLOR")), QStringLiteral("1"));
    EXPECT_EQ(sanitized.value(QStringLiteral("LANG")), QStringLiteral("C.UTF-8"));
}
