#include "core/CommandStack.hpp"

namespace ccos::core {

bool CommandStack::execute(std::unique_ptr<Command> command) {
    if (!command || !command->execute()) return false;
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
    if (undoStack_.size() > maxHistory_) undoStack_.erase(undoStack_.begin());
    return true;
}

bool CommandStack::undo() {
    if (undoStack_.empty()) return false;
    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();
    command->undo();
    redoStack_.push_back(std::move(command));
    return true;
}

bool CommandStack::redo() {
    if (redoStack_.empty()) return false;
    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();
    if (!command->execute()) return false;
    undoStack_.push_back(std::move(command));
    return true;
}

void CommandStack::clear() {
    undoStack_.clear();
    redoStack_.clear();
}

}
