#pragma once
#include <QString>

namespace ccos::core {
class Command {
public:
    virtual ~Command() = default;
    virtual bool execute() = 0;
    virtual void undo() = 0;
    virtual QString name() const = 0;
};
}
