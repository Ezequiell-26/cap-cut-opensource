#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace ccos::ai {

struct AICommand {
    QString operation;
    QString target;
    QJsonObject arguments;
    bool destructive = false;
};

struct AICommandPlan {
    QString summary;
    QVector<AICommand> commands;
    bool requiresConfirmation = false;

    [[nodiscard]] bool validate(QString* error = nullptr) const;
    [[nodiscard]] QJsonObject preview() const;
};

} // namespace ccos::ai
