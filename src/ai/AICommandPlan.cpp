#include "ai/AICommandPlan.hpp"

#include <QJsonArray>
#include <QSet>

namespace ccos::ai {
namespace {

const QSet<QString> kAllowedOperations = {
    QStringLiteral("set_project_name"),
    QStringLiteral("timeline_slip"),
    QStringLiteral("timeline_ripple_delete"),
    QStringLiteral("export")
};

}

bool AICommandPlan::validate(QString* error) const {
    if (summary.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("AI command plan summary is required");
        return false;
    }
    if (commands.size() > 128) {
        if (error) *error = QStringLiteral("AI command plan contains too many commands");
        return false;
    }
    for (const auto& command : commands) {
        if (!kAllowedOperations.contains(command.operation)) {
            if (error) *error = QStringLiteral("Unsupported AI operation: %1").arg(command.operation);
            return false;
        }
        if (command.target.size() > 512 || command.arguments.size() > 64) {
            if (error) *error = QStringLiteral("AI command payload exceeds safety limits");
            return false;
        }
        if (command.destructive && !requiresConfirmation) {
            if (error) *error = QStringLiteral("Destructive AI commands require explicit confirmation");
            return false;
        }
    }
    return true;
}

QJsonObject AICommandPlan::preview() const {
    QJsonArray commandsJson;
    for (const auto& command : commands) {
        commandsJson.append(QJsonObject{
            {QStringLiteral("operation"), command.operation},
            {QStringLiteral("target"), command.target},
            {QStringLiteral("arguments"), command.arguments},
            {QStringLiteral("destructive"), command.destructive}
        });
    }
    return QJsonObject{
        {QStringLiteral("summary"), summary},
        {QStringLiteral("requiresConfirmation"), requiresConfirmation},
        {QStringLiteral("commands"), commandsJson}
    };
}

} // namespace ccos::ai
