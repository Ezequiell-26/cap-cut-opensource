#pragma once

#include <QString>
#include <QUuid>
#include <QVariant>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace ccos::core {

/**
 * @brief Nivel de riesgo de una operación Command.
 * 
 * Determina el nivel de validación y confirmación requerido.
 */
enum class CommandRiskLevel {
    Low,        // Operaciones reversibles simples (cambiar propiedad)
    Medium,     // Operaciones reversibles complejas (mover clip)
    High,       // Operaciones destructivas (eliminar clip)
    Critical    // Operaciones irreversibles (eliminar proyecto)
};

/**
 * @brief Estado de validación de un Command.
 */
struct CommandValidationResult {
    bool isValid = false;
    QString errorMessage;
    QStringList warnings;
    
    static CommandValidationResult success() {
        CommandValidationResult result;
        result.isValid = true;
        return result;
    }
    
    static CommandValidationResult failure(const QString &reason) {
        CommandValidationResult result;
        result.isValid = false;
        result.errorMessage = reason;
        return result;
    }
};

/**
 * @brief Información de afectación para tracking de cambios.
 */
struct CommandAffectedItems {
    QStringList projectIds;
    QStringList timelineIds;
    QStringList trackIds;
    QStringList clipIds;
    QStringList assetIds;
    QStringList textLayerIds;
    QStringList effectIds;
    
    bool isEmpty() const {
        return projectIds.isEmpty() && timelineIds.isEmpty() && 
               trackIds.isEmpty() && clipIds.isEmpty() && 
               assetIds.isEmpty() && textLayerIds.isEmpty() && effectIds.isEmpty();
    }
    
    void merge(const CommandAffectedItems &other) {
        for (const auto &id : other.projectIds) if (!projectIds.contains(id)) projectIds.append(id);
        for (const auto &id : other.timelineIds) if (!timelineIds.contains(id)) timelineIds.append(id);
        for (const auto &id : other.trackIds) if (!trackIds.contains(id)) trackIds.append(id);
        for (const auto &id : other.clipIds) if (!clipIds.contains(id)) clipIds.append(id);
        for (const auto &id : other.assetIds) if (!assetIds.contains(id)) assetIds.append(id);
        for (const auto &id : other.textLayerIds) if (!textLayerIds.contains(id)) textLayerIds.append(id);
        for (const auto &id : other.effectIds) if (!effectIds.contains(id)) effectIds.append(id);
    }
};

/**
 * @brief Clase base para todos los Commands del sistema.
 * 
 * CARACTERÍSTICAS:
 * - Execute/Undo/Redo consistentes
 * - Validación pre-ejecución
 * - Descripción humana legible
 * - Tracking de items afectados
 * - Nivel de riesgo para confirmaciones
 * - Agrupamiento transaccional opcional
 * - Estado interno para idempotencia
 */
class Command {
public:
    virtual ~Command() = default;
    
    /**
     * @brief Ejecuta la operación del command.
     * @return true si la ejecución fue exitosa
     * 
     * Debe ser idempotente: ejecutar múltiples veces debe tener el mismo efecto que una vez.
     */
    virtual bool execute() = 0;
    
    /**
     * @brief Revierte la operación del command.
     * 
     * Debe dejar el sistema en el estado exacto anterior a execute().
     */
    virtual void undo() = 0;
    
    /**
     * @brief Reejecuta la operación después de un undo.
     * 
     * Por defecto llama a execute(), pero puede sobrescribirse para optimización.
     */
    virtual bool redo() { return execute(); }
    
    /**
     * @brief Nombre descriptivo del command para UI y logs.
     */
    virtual QString name() const = 0;
    
    /**
     * @brief Descripción detallada para logging y debugging.
     */
    virtual QString description() const { return name(); }
    
    /**
     * @brief Valida que el command puede ejecutarse.
     * @return Resultado de validación
     * 
     * Se llama ANTES de execute(). Si falla, execute() no se llama.
     */
    virtual CommandValidationResult validate() const {
        return CommandValidationResult::success();
    }
    
    /**
     * @brief Obtiene los items afectados por este command.
     */
    virtual CommandAffectedItems affectedItems() const {
        return CommandAffectedItems{};
    }
    
    /**
     * @brief Obtiene el nivel de riesgo del command.
     */
    virtual CommandRiskLevel riskLevel() const {
        return CommandRiskLevel::Medium;
    }
    
    /**
     * @brief ID único del command para tracking.
     */
    QString id() const { return m_id; }
    
    /**
     * @brief Timestamp de creación del command.
     */
    qint64 timestamp() const { return m_timestamp; }
    
    /**
     * @brief Verifica si el command ya fue ejecutado.
     */
    bool isExecuted() const { return m_executed; }
    
    /**
     * @brief Grupo transaccional al que pertenece (opcional).
     */
    QString transactionGroup() const { return m_transactionGroup; }
    
    /**
     * @brief Establece el grupo transaccional.
     */
    void setTransactionGroup(const QString &group) { m_transactionGroup = group; }

protected:
    Command() 
        : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , m_timestamp(QDateTime::currentMSecsSinceEpoch()) 
    {}
    
    explicit Command(const QString &id)
        : m_id(id)
        , m_timestamp(QDateTime::currentMSecsSinceEpoch())
    {}
    
    void markExecuted() { m_executed = true; }
    void markUndone() { m_executed = false; }
    
private:
    QString m_id;
    qint64 m_timestamp;
    bool m_executed = false;
    QString m_transactionGroup;
};

/**
 * @brief Macro helper para definir Commands comunes.
 */
#define DECLARE_COMMAND(className, baseName) \
    class className final : public ccos::core::Command { \
    public: \
        QString name() const override { return QStringLiteral(baseName); } \
        using Command::Command; \
    private: \
        bool m_executed = false; \
    }

} // namespace ccos::core
