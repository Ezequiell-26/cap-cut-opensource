#pragma once
#include "core/Command.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace ccos::core {
class CommandStack {
public:
    explicit CommandStack(std::size_t maxHistory = 200) : maxHistory_(maxHistory) {}
    bool execute(std::unique_ptr<Command> command);
    bool undo();
    bool redo();
    void clear();
    [[nodiscard]] bool canUndo() const noexcept { return !undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack_.empty(); }
private:
    std::size_t maxHistory_;
    std::vector<std::unique_ptr<Command>> undoStack_;
    std::vector<std::unique_ptr<Command>> redoStack_;
};
}
