#include "render/graph/RenderGraph.hpp"

#include <gtest/gtest.h>

#include <limits>

using namespace ccos::render;

TEST(RenderGraphSafetyTest, RejectsDuplicateNodeIds) {
    RenderGraph graph;
    graph.createNode<SourceNode>("source", "/video.mp4");
    EXPECT_THROW(graph.createNode<TransformNode>("source"), DuplicateNodeException);
}

TEST(RenderGraphSafetyTest, RejectsSelfConnection) {
    RenderGraph graph;
    graph.createNode<SourceNode>("source", "/video.mp4");
    EXPECT_THROW(graph.connect("source", "source"), InvalidConnectionException);
}

TEST(RenderGraphSafetyTest, RejectsConnectionThatCreatesCycle) {
    RenderGraph graph;
    graph.createNode<SourceNode>("a", "/a.mp4");
    graph.createNode<TransformNode>("b");
    graph.createNode<CompositeNode>("c");

    graph.connect("a", "b");
    graph.connect("b", "c");
    EXPECT_THROW(graph.connect("c", "a"), CycleDetectedException);
    EXPECT_TRUE(graph.getNode("a")->inputs().empty());
}

TEST(RenderGraphSafetyTest, DisconnectRemovesNodeInput) {
    RenderGraph graph;
    graph.createNode<SourceNode>("a", "/a.mp4");
    graph.createNode<TransformNode>("b");

    graph.connect("a", "b");
    ASSERT_EQ(graph.getNode("b")->inputs().size(), 1U);
    graph.disconnect("a", "b");
    EXPECT_TRUE(graph.getNode("b")->inputs().empty());
}

TEST(RenderGraphSafetyTest, ExecutionOrderIsDeterministic) {
    RenderGraph graph;
    graph.createNode<SourceNode>("a", "/a.mp4");
    graph.createNode<TransformNode>("b");
    graph.createNode<EffectNode>("c", "blur");

    graph.connect("a", "b");
    graph.connect("b", "c");

    const auto order = graph.getExecutionOrder();
    ASSERT_EQ(order.size(), 3U);
    EXPECT_EQ(order[0], "a");
    EXPECT_EQ(order[1], "b");
    EXPECT_EQ(order[2], "c");
}

TEST(RenderGraphSafetyTest, RejectsInvalidRationalTime) {
    EXPECT_THROW(RationalTime(1, 0), std::invalid_argument);
    EXPECT_THROW(RationalTime::fromSeconds(std::numeric_limits<double>::infinity()), std::invalid_argument);
}
