#include "core/CommandStack.hpp"

#include <gtest/gtest.h>

namespace {

class CounterCommand final : public ccos::core::Command {
public:
    explicit CounterCommand(int& value) : value_(value) {}
    bool execute() override {
        ++value_;
        return true;
    }
    void undo() override { --value_; }
    QString name() const override { return QStringLiteral("Counter"); }

private:
    int& value_;
};

class FailingRedoCommand final : public ccos::core::Command {
public:
    explicit FailingRedoCommand(int& value) : value_(value) {}

    bool execute() override {
        ++value_;
        return true;
    }

    bool redo() override {
        return false;
    }

    void undo() override { --value_; }
    QString name() const override { return QStringLiteral("FailingRedo"); }

private:
    int& value_;
};

} // namespace

TEST(CommandStackTests, ExecutesUndoesAndRedoes) {
    int value = 0;
    ccos::core::CommandStack stack;
    EXPECT_TRUE(stack.execute(std::make_unique<CounterCommand>(value)));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(stack.undo());
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(stack.redo());
    EXPECT_EQ(value, 1);
}

TEST(CommandStackTests, FailedRedoKeepsCommandAvailable) {
    int value = 0;
    ccos::core::CommandStack stack;
    EXPECT_TRUE(stack.execute(std::make_unique<FailingRedoCommand>(value)));
    ASSERT_TRUE(stack.undo());
    EXPECT_EQ(value, 0);
    EXPECT_FALSE(stack.redo());
    EXPECT_TRUE(stack.canRedo());
    EXPECT_FALSE(stack.canUndo());
}
