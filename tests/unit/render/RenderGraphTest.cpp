#include <gtest/gtest.h>
#include "../src/render/graph/RenderGraph.hpp"
#include "../src/render/scheduler/RenderScheduler.hpp"

using namespace ccos::render;

// Tests para RenderGraphTypes y RenderGraph
class RenderGraphTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RenderGraphTest, CreateSourceNode) {
    auto graph = std::make_shared<RenderGraph>();
    auto source = graph->createNode<SourceNode>("source1", "/path/to/video.mp4");
    
    EXPECT_EQ(source->id(), "source1");
    EXPECT_EQ(source->type(), NodeType::Source);
    EXPECT_EQ(source->filePath(), "/path/to/video.mp4");
    EXPECT_TRUE(source->validate());
}

TEST_F(RenderGraphTest, SourceNodeValidation) {
    auto graph = std::make_shared<RenderGraph>();
    auto source = graph->createNode<SourceNode>("source1", "");
    
    EXPECT_FALSE(source->validate()); // Path vacío debe fallar
}

TEST_F(RenderGraphTest, CreateTransformNode) {
    auto graph = std::make_shared<RenderGraph>();
    auto transform = graph->createNode<TransformNode>("transform1");
    
    transform->setPosition(100.0f, 200.0f);
    transform->setScale(2.0f, 2.0f);
    transform->setRotation(45.0f);
    
    EXPECT_TRUE(transform->validate());
}

TEST_F(RenderGraphTest, TransformNodeInvalidScale) {
    auto graph = std::make_shared<RenderGraph>();
    auto transform = graph->createNode<TransformNode>("transform1");
    
    transform->setScale(0.0f, 1.0f); // Scale X inválido
    
    EXPECT_FALSE(transform->validate());
}

TEST_F(RenderGraphTest, ConnectNodes) {
    auto graph = std::make_shared<RenderGraph>();
    auto source = graph->createNode<SourceNode>("source1", "/path/video.mp4");
    auto transform = graph->createNode<TransformNode>("transform1");
    
    graph->connect("source1", "transform1");
    
    auto transformNode = std::dynamic_pointer_cast<TransformNode>(graph->getNode("transform1"));
    EXPECT_EQ(transformNode->inputs().size(), 1);
    EXPECT_EQ(transformNode->inputs()[0], "source1");
}

TEST_F(RenderGraphTest, DetectCycle) {
    auto graph = std::make_shared<RenderGraph>();
    graph->createNode<SourceNode>("node1", "/path/video.mp4");
    graph->createNode<TransformNode>("node2");
    graph->createNode<CompositeNode>("node3");
    
    graph->connect("node1", "node2");
    graph->connect("node2", "node3");
    graph->connect("node3", "node1"); // Crear ciclo
    
    EXPECT_TRUE(graph->hasCycle());
    
    auto validation = graph->validate();
    EXPECT_FALSE(validation.isValid);
    EXPECT_FALSE(validation.errors.empty());
}

TEST_F(RenderGraphTest, GetExecutionOrder) {
    auto graph = std::make_shared<RenderGraph>();
    graph->createNode<SourceNode>("source1", "/path/video.mp4");
    graph->createNode<TransformNode>("transform1");
    graph->createNode<EffectNode>("effect1", "blur");
    graph->createNode<CompositeNode>("composite1");
    
    graph->connect("source1", "transform1");
    graph->connect("transform1", "effect1");
    graph->connect("effect1", "composite1");
    
    auto order = graph->getExecutionOrder();
    
    EXPECT_EQ(order.size(), 4);
    EXPECT_EQ(order[0], "source1");
    EXPECT_EQ(order[1], "transform1");
    EXPECT_EQ(order[2], "effect1");
    EXPECT_EQ(order[3], "composite1");
}

TEST_F(RenderGraphTest, GetRootNodes) {
    auto graph = std::make_shared<RenderGraph>();
    graph->createNode<SourceNode>("source1", "/path/video1.mp4");
    graph->createNode<SourceNode>("source2", "/path/video2.mp4");
    graph->createNode<TransformNode>("transform1");
    
    graph->connect("source1", "transform1");
    // source2 está desconectado
    
    auto roots = graph->getRootNodes();
    EXPECT_EQ(roots.size(), 2);
    EXPECT_NE(std::find(roots.begin(), roots.end(), "source1"), roots.end());
    EXPECT_NE(std::find(roots.begin(), roots.end(), "source2"), roots.end());
}

TEST_F(RenderGraphTest, GetLeafNodes) {
    auto graph = std::make_shared<RenderGraph>();
    graph->createNode<SourceNode>("source1", "/path/video.mp4");
    graph->createNode<TransformNode>("transform1");
    graph->createNode<CompositeNode>("composite1");
    
    graph->connect("source1", "transform1");
    graph->connect("transform1", "composite1");
    
    auto leaves = graph->getLeafNodes();
    EXPECT_EQ(leaves.size(), 1);
    EXPECT_EQ(leaves[0], "composite1");
}

TEST_F(RenderGraphTest, RemoveNode) {
    auto graph = std::make_shared<RenderGraph>();
    graph->createNode<SourceNode>("source1", "/path/video.mp4");
    graph->createNode<TransformNode>("transform1");
    
    graph->connect("source1", "transform1");
    
    graph->removeNode("source1");
    
    EXPECT_FALSE(graph->hasNode("source1"));
    EXPECT_TRUE(graph->hasNode("transform1"));
    EXPECT_TRUE(graph->getNode("transform1")->inputs().empty());
}

TEST_F(RenderGraphTest, NodeNotFound) {
    auto graph = std::make_shared<RenderGraph>();
    
    EXPECT_THROW(graph->getNode("nonexistent"), NodeNotFoundException);
}

TEST_F(RenderGraphTest, BuilderPattern) {
    auto builder = std::make_shared<RenderGraphBuilder>(std::make_shared<RenderGraph>());
    
    auto graph = builder->addSource("source1", "/path/video.mp4")
        .addTransform("transform1")
        .addEffect("effect1", "sharpen")
        .addComposite("composite1")
        .connect("source1", "transform1")
        .connect("transform1", "effect1")
        .connect("effect1", "composite1")
        .setResolution(1920, 1080)
        .setFrameRate(30.0)
        .setDurationSeconds(10.0)
        .build();
    
    EXPECT_TRUE(graph->validate().isValid);
    EXPECT_EQ(graph->outputResolution(), Resolution::HD());
    EXPECT_DOUBLE_EQ(graph->frameRate(), 30.0);
}

TEST_F(RenderGraphTest, InvalidConnection) {
    auto graph = std::make_shared<RenderGraph>();
    graph->createNode<SourceNode>("source1", "/path/video.mp4");
    
    EXPECT_THROW(graph->connect("source1", "nonexistent"), NodeNotFoundException);
}

TEST_F(RenderGraphTest, SetAndGetParameters) {
    auto graph = std::make_shared<RenderGraph>();
    auto transform = graph->createNode<TransformNode>("transform1");
    
    transform->setParameter("posX", 100.0f);
    transform->setParameter("posY", 200.0f);
    transform->setParameter("scaleX", 1.5f);
    
    auto posX = transform->getParameter("posX");
    auto posY = transform->getParameter("posY");
    auto scaleX = transform->getParameter("scaleX");
    
    ASSERT_TRUE(posX.has_value());
    ASSERT_TRUE(posY.has_value());
    ASSERT_TRUE(scaleX.has_value());
    
    EXPECT_FLOAT_EQ(std::get<float>(*posX), 100.0f);
    EXPECT_FLOAT_EQ(std::get<float>(*posY), 200.0f);
    EXPECT_FLOAT_EQ(std::get<float>(*scaleX), 1.5f);
}

// Tests para RenderScheduler
class RenderSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

class MockRenderTask : public RenderTask {
public:
    MockRenderTask(const std::string& id, int framesToRender = 10)
        : RenderTask(id), framesToRender_(framesToRender) {}
    
    void execute() override {
        for (int i = 0; i < framesToRender_ && !isCancelled(); ++i) {
            if (state() == RenderTaskState::Paused) {
                while (state() == RenderTaskState::Paused && !isCancelled()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            
            updateProgress(static_cast<double>(i + 1) / framesToRender_, i + 1, framesToRender_);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (!isCancelled()) {
            RenderResult result;
            result.success = true;
            result.framesRendered = framesToRender_;
            result.outputPath = "/output/result.mp4";
            reportCompletion(result);
        }
    }
    
private:
    int framesToRender_;
};

TEST_F(RenderSchedulerTest, CreateTask) {
    auto task = std::make_shared<MockRenderTask>("task1", 10);
    
    EXPECT_EQ(task->id(), "task1");
    EXPECT_EQ(task->state(), RenderTaskState::Pending);
    EXPECT_FALSE(task->isCancelled());
}

TEST_F(RenderSchedulerTest, CancelTask) {
    auto task = std::make_shared<MockRenderTask>("task1", 100);
    
    task->cancel();
    
    EXPECT_TRUE(task->isCancelled());
    EXPECT_EQ(task->state(), RenderTaskState::Cancelled);
}

TEST_F(RenderSchedulerTest, PauseAndResumeTask) {
    auto task = std::make_shared<MockRenderTask>("task1", 100);
    task->state_ = RenderTaskState::Running;
    
    task->pause();
    EXPECT_EQ(task->state(), RenderTaskState::Paused);
    
    task->resume();
    EXPECT_EQ(task->state(), RenderTaskState::Running);
}

TEST_F(RenderSchedulerTest, ProgressCallback) {
    auto task = std::make_shared<MockRenderTask>("task1", 10);
    
    bool callbackCalled = false;
    double lastProgress = 0.0;
    
    task->setProgressCallback([&](const RenderProgress& progress) {
        callbackCalled = true;
        lastProgress = progress.progress;
    });
    
    task->execute();
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_DOUBLE_EQ(lastProgress, 1.0);
}

TEST_F(RenderSchedulerTest, CompletionCallback) {
    auto task = std::make_shared<MockRenderTask>("task1", 5);
    
    bool completed = false;
    bool success = false;
    
    task->setCompletionCallback([&](const RenderResult& result) {
        completed = true;
        success = result.success;
    });
    
    task->execute();
    
    EXPECT_TRUE(completed);
    EXPECT_TRUE(success);
}

TEST_F(RenderSchedulerTest, RenderQueueSingleton) {
    auto& queue1 = RenderQueue::instance();
    auto& queue2 = RenderQueue::instance();
    
    EXPECT_EQ(&queue1, &queue2);
}

TEST_F(RenderSchedulerTest, AddTaskToQueue) {
    auto& queue = RenderQueue::instance();
    queue.stop(); // Detener procesamiento automático
    
    auto task = std::make_shared<MockRenderTask>("task1", 10);
    queue.addTask(task);
    
    EXPECT_EQ(queue.pendingCount(), 1);
    EXPECT_EQ(queue.allTasks().size(), 1);
}

TEST_F(RenderSchedulerTest, RemoveTaskFromQueue) {
    auto& queue = RenderQueue::instance();
    queue.stop();
    
    auto task = std::make_shared<MockRenderTask>("task1", 10);
    queue.addTask(task);
    
    queue.removeTask("task1");
    
    EXPECT_EQ(queue.pendingCount(), 0);
    EXPECT_EQ(queue.allTasks().size(), 0);
}

TEST_F(RenderSchedulerTest, QueueCounts) {
    auto& queue = RenderQueue::instance();
    queue.stop();
    
    auto task1 = std::make_shared<MockRenderTask>("task1", 10);
    auto task2 = std::make_shared<MockRenderTask>("task2", 10);
    
    queue.addTask(task1);
    queue.addTask(task2);
    
    EXPECT_EQ(queue.pendingCount(), 2);
    EXPECT_EQ(queue.runningCount(), 0);
    EXPECT_EQ(queue.completedCount(), 0);
}

TEST_F(RenderSchedulerTest, SetMaxConcurrentTasks) {
    auto& queue = RenderQueue::instance();
    
    queue.setMaxConcurrentTasks(4);
    EXPECT_EQ(queue.maxConcurrentTasks(), 4);
    
    queue.setMaxConcurrentTasks(1);
    EXPECT_EQ(queue.maxConcurrentTasks(), 1);
}

TEST_F(RenderSchedulerTest, PendingTasksFilter) {
    auto& queue = RenderQueue::instance();
    queue.stop();
    
    auto task1 = std::make_shared<MockRenderTask>("task1", 10);
    auto task2 = std::make_shared<MockRenderTask>("task2", 10);
    
    queue.addTask(task1);
    queue.addTask(task2);
    
    auto pending = queue.pendingTasks();
    EXPECT_EQ(pending.size(), 2);
}

TEST_F(RenderSchedulerTest, TaskSettings) {
    auto task = std::make_shared<MockRenderTask>("task1", 10);
    
    RenderSettings settings;
    settings.codec = "h265";
    settings.quality = 28;
    settings.outputFormat = "mkv";
    
    task->setSettings(settings);
    
    const auto& retrieved = task->settings();
    EXPECT_EQ(retrieved.codec, "h265");
    EXPECT_EQ(retrieved.quality, 28);
    EXPECT_EQ(retrieved.outputFormat, "mkv");
}
