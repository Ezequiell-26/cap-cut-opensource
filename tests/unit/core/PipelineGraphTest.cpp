#include "core/PipelineGraph.hpp"

#include <gtest/gtest.h>

#include <QStringList>

TEST(PipelineGraphTest, RejectsEmptyAndDuplicateTasks) {
    ccos::core::PipelineGraph graph;
    EXPECT_FALSE(graph.addTask({}, [] { return true; }));
    EXPECT_TRUE(graph.addTask(QStringLiteral("decode"), [] { return true; }));
    EXPECT_FALSE(graph.addTask(QStringLiteral("decode"), [] { return true; }));
}

TEST(PipelineGraphTest, ExecutesDependenciesInOrder) {
    ccos::core::PipelineGraph graph;
    QStringList order;

    ASSERT_TRUE(graph.addTask(QStringLiteral("decode"), [&] {
        order.append(QStringLiteral("decode"));
        return true;
    }));
    ASSERT_TRUE(graph.addTask(QStringLiteral("proxy"), [&] {
        EXPECT_EQ(order, QStringList{QStringLiteral("decode")});
        order.append(QStringLiteral("proxy"));
        return true;
    }));
    ASSERT_TRUE(graph.addTask(QStringLiteral("render"), [&] {
        EXPECT_EQ(order, QStringList{QStringLiteral("decode"), QStringLiteral("proxy")});
        order.append(QStringLiteral("render"));
        return true;
    }));

    ASSERT_TRUE(graph.addDependency(QStringLiteral("decode"), QStringLiteral("proxy")));
    ASSERT_TRUE(graph.addDependency(QStringLiteral("proxy"), QStringLiteral("render")));

    QString error;
    EXPECT_TRUE(graph.validate(&error)) << error.toStdString();
    EXPECT_TRUE(graph.run(&error)) << error.toStdString();
    EXPECT_EQ(order, QStringList({QStringLiteral("decode"), QStringLiteral("proxy"), QStringLiteral("render")}));
}

TEST(PipelineGraphTest, RejectsCycles) {
    ccos::core::PipelineGraph graph;
    ASSERT_TRUE(graph.addTask(QStringLiteral("a"), [] { return true; }));
    ASSERT_TRUE(graph.addTask(QStringLiteral("b"), [] { return true; }));
    ASSERT_TRUE(graph.addDependency(QStringLiteral("a"), QStringLiteral("b")));
    EXPECT_FALSE(graph.addDependency(QStringLiteral("b"), QStringLiteral("a")));
}

TEST(PipelineGraphTest, FailingTaskFailsPipeline) {
    ccos::core::PipelineGraph graph;
    bool dependentExecuted = false;

    ASSERT_TRUE(graph.addTask(QStringLiteral("source"), [] { return false; }));
    ASSERT_TRUE(graph.addTask(QStringLiteral("encode"), [&] {
        dependentExecuted = true;
        return true;
    }));
    ASSERT_TRUE(graph.addDependency(QStringLiteral("source"), QStringLiteral("encode")));

    QString error;
    EXPECT_FALSE(graph.run(&error));
    EXPECT_FALSE(dependentExecuted);
    EXPECT_FALSE(error.isEmpty());
}

TEST(PipelineGraphTest, AsyncExecutionUsesSnapshot) {
    ccos::core::PipelineGraph graph;
    ASSERT_TRUE(graph.addTask(QStringLiteral("task"), [] { return true; }));

    auto future = graph.runAsync();
    graph.clear();
    ASSERT_TRUE(future.waitForFinished());
    EXPECT_TRUE(future.result());
}

TEST(PipelineGraphTest, TaskIdsAreStableAndSorted) {
    ccos::core::PipelineGraph graph;
    ASSERT_TRUE(graph.addTask(QStringLiteral("render"), [] { return true; }));
    ASSERT_TRUE(graph.addTask(QStringLiteral("decode"), [] { return true; }));
    ASSERT_TRUE(graph.addTask(QStringLiteral("proxy"), [] { return true; }));

    EXPECT_EQ(graph.taskIds(), QStringList({QStringLiteral("decode"), QStringLiteral("proxy"), QStringLiteral("render")}));
}
