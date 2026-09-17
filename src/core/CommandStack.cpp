#include "core/CommandStack.hpp"
#include <QDateTime>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(ccos_core_command, "ccos.core.command")

namespace ccos::core {

bool CommandStack::execute(std::unique_ptr<Command> command) {
    if (!command) {
        qCWarning(ccos_core_command) << "Attempted to execute null command";
        return false;
    }
    
    // Validar command antes de ejecutar
    auto validationResult = command->validate();
    if (!validationResult.isValid) {
        qCWarning(ccos_core_command) << "Command validation failed:" << validationResult.errorMessage;
        return false;
    }
    
    // Ejecutar warnings si existen
    for (const QString &warning : validationResult.warnings) {
        qCWarning(ccos_core_command) << "Command warning:" << warning;
    }
    
    if (!command->execute()) {
        qCWarning(ccos_core_command) << "Command execution failed:" << command->name();
        return false;
    }
    
    command->markExecuted();
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
    
    if (undoStack_.size() > maxHistory_) {
        undoStack_.erase(undoStack_.begin());
    }
    
    qCInfo(ccos_core_command) << "Command executed:" << undoStack_.back()->name() 
                              << "id:" << undoStack_.back()->id();
    return true;
}

bool CommandStack::undo() {
    if (undoStack_.empty()) {
        return false;
    }
    
    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();
    
    command->undo();
    command->markUndone();
    
    redoStack_.push_back(std::move(command));
    
    qCInfo(ccos_core_command) << "Command undone:" << redoStack_.back()->name();
    return true;
}

bool CommandStack::redo() {
    if (redoStack_.empty()) {
        return false;
    }
    
    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();
    
    if (!command->redo()) {
        qCWarning(ccos_core_command) << "Command redo failed:" << command->name();
        return false;
    }
    
    command->markExecuted();
    undoStack_.push_back(std::move(command));
    
    qCInfo(ccos_core_command) << "Command redone:" << undoStack_.back()->name();
    return true;
}

void CommandStack::clear() {
    int undoCount = static_cast<int>(undoStack_.size());
    int redoCount = static_cast<int>(redoStack_.size());
    
    undoStack_.clear();
    redoStack_.clear();
    
    if (undoCount > 0 || redoCount > 0) {
        qCInfo(ccos_core_command) << "Command stack cleared. Undone:" << undoCount << "Redone:" << redoCount;
    }
}

} // namespace ccos::core
