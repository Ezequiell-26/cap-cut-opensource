#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVariant>

#include <memory>

namespace ccos::core {

enum class CommandRiskLevel {
    Low,
    Medium,
    High,
    Critical
};

struct CommandValidationResult {
    bool isValid = false;
    QString errorMessage;
    QStringList warnings;

    static CommandValidationResult success() {
        CommandValidationResult result;
        result.isValid = true;
        return result;
    }

    static CommandValidationResult failure(const QString& reason) {
        CommandValidationResult result;
        result.isValid = false;
        result.errorMessage = reason;
        return result;
    }
};

struct CommandAffectedItems {
    QStringList projectIds;
    QStringList timelineIds;
    QStringList trackIds;
    QStringList clipIds;
    QStringList assetIds;
    QStringList textLayerIds;
    QStringList effectIds;

    [[nodiscard]] bool isEmpty() const {
        return projectIds.isEmpty() && timelineIds.isEmpty() && trackIds.isEmpty() &&
               clipIds.isEmpty() && assetIds.isEmpty() && textLayerIds.isEmpty() && effectIds.isEmpty();
    }

    void merge(const CommandAffectedItems& other) {
        for (const auto& id : other.projectIds) if (!projectIds.contains(id)) projectIds.append(id);
        for (const auto& id : other.timelineIds) if (!timelineIds.contains(id)) timelineIds.append(id);
        for (const auto& id : other.trackIds) if (!trackIds.contains(id)) trackIds.append(id);
        for (const auto& id : other.clipIds) if (!clipIds.contains(id)) clipIds.append(id);
        for (const auto& id : other.assetIds) if (!assetIds.contains(id)) assetIds.append(id);
        for (const auto& id : other.textLayerIds) if (!textLayerIds.contains(id)) textLayerIds.append(id);
        for (const auto& id : other.effectIds) if (!effectIds.contains(id)) effectIds.append(id);
    }
};

class Command {
public:
    virtual ~Command() = default;

    virtual bool execute() = 0;
    virtual void undo() = 0;
    virtual bool redo() { return execute(); }

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual QString description() const { return name(); }
    [[nodiscard]] virtual CommandValidationResult validate() const {
        return CommandValidationResult::success();
    }
    [[nodiscard]] virtual CommandAffectedItems affectedItems() const {
        return {};
    }
    [[nodiscard]] virtual CommandRiskLevel riskLevel() const {
        return CommandRiskLevel::Medium;
    }

    [[nodiscard]] const QString& id() const noexcept { return m_id; }
    [[nodiscard]] qint64 timestamp() const noexcept { return m_timestamp; }
    [[nodiscard]] bool isExecuted() const noexcept { return m_executed; }
    [[nodiscard]] const QString& transactionGroup() const noexcept { return m_transactionGroup; }
    void setTransactionGroup(const QString& group) { m_transactionGroup = group; }

protected:
    Command()
        : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , m_timestamp(QDateTime::currentMSecsSinceEpoch()) {}

    explicit Command(const QString& id)
        : m_id(id)
        , m_timestamp(QDateTime::currentMSecsSinceEpoch()) {}

    void markExecuted() noexcept { m_executed = true; }
    void markUndone() noexcept { m_executed = false; }

private:
    QString m_id;
    qint64 m_timestamp = 0;
    bool m_executed = false;
    QString m_transactionGroup;
};

} // namespace ccos::core
