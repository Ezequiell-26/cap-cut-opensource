#include "core/CommandStack.hpp"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(ccos_core_command, "ccos.core.command")

namespace ccos::core {

bool CommandStack::execute(std::unique_ptr<Command> command) {
    if (!command) {
        qCWarning(ccos_core_command) << "Attempted to execute null command";
        return false;
    }

    const auto validationResult = command->validate();
    if (!validationResult.isValid) {
        qCWarning(ccos_core_command) << "Command validation failed:" << validationResult.errorMessage;
        return false;
    }

    for (const auto& warning : validationResult.warnings) {
        qCWarning(ccos_core_command) << "Command warning:" << warning;
    }

    if (!command->execute()) {
        qCWarning(ccos_core_command) << "Command execution failed:" << command->name();
        return false;
    }

    command->markExecuted();
    redoStack_.clear();

    if (maxHistory_ > 0) {
        undoStack_.push_back(std::move(command));
        if (undoStack_.size() > maxHistory_) undoStack_.erase(undoStack_.begin());
        qCInfo(ccos_core_command) << "Command executed:" << undoStack_.back()->name()
                                  << "id:" << undoStack_.back()->id();
    } else {
        qCInfo(ccos_core_command) << "Command executed with history disabled:" << command->name()
                                  << "id:" << command->id();
    }
    return true;
}

bool CommandStack::undo() {
    if (undoStack_.empty()) return false;

    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();

    command->undo();
    command->markUndone();
    redoStack_.push_back(std::move(command));

    qCInfo(ccos_core_command) << "Command undone:" << redoStack_.back()->name();
    return true;
}

bool CommandStack::redo() {
    if (redoStack_.empty()) return false;

    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();

    if (!command->redo()) {
        qCWarning(ccos_core_command) << "Command redo failed:" << command->name();
        redoStack_.push_back(std::move(command));
        return false;
    }

    command->markExecuted();
    if (maxHistory_ > 0) {
        undoStack_.push_back(std::move(command));
        if (undoStack_.size() > maxHistory_) undoStack_.erase(undoStack_.begin());
        qCInfo(ccos_core_command) << "Command redone:" << undoStack_.back()->name();
    } else {
        qCInfo(ccos_core_command) << "Command redone with history disabled:" << command->name();
    }
    return true;
}

void CommandStack::clear() {
    const int undoCount = static_cast<int>(undoStack_.size());
    const int redoCount = static_cast<int>(redoStack_.size());
    undoStack_.clear();
    redoStack_.clear();

    if (undoCount > 0 || redoCount > 0) {
        qCInfo(ccos_core_command) << "Command stack cleared. Undone:" << undoCount
                                  << "Redone:" << redoCount;
    }
}

} // namespace ccos::core
