#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QMap>
#include <QSet>
#include <QMutex>

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace ccos::core {

/**
 * @brief Nivel de riesgo para operaciones de AI Tools.
 */
enum class ToolRiskLevel {
    READ_ONLY,           // Solo lectura, sin efectos secundarios
    SAFE_MUTATION,       // Modificaciones reversibles simples
    SENSITIVE_MUTATION,  // Modificaciones que requieren validación
    DESTRUCTIVE          // Operaciones destructivas o irreversibles
};

/**
 * @brief Esquema de parámetro para validación de inputs.
 */
struct ToolParameterSchema {
    QString name;
    QString type; // "string", "number", "boolean", "array", "object"
    QString description;
    bool required = false;
    QVariant defaultValue;
    QStringList enumValues; // Para tipos enum
    int minLength = -1;
    int maxLength = -1;
    double minimum = std::numeric_limits<double>::lowest();
    double maximum = std::numeric_limits<double>::max();
    QString pattern; // Regex para strings
    
    bool validate(const QVariant &value) const;
    QString validationError(const QVariant &value) const;
};

/**
 * @brief Resultado de la ejecución de una tool.
 */
struct ToolResult {
    bool success = false;
    QVariant data;
    QString errorMessage;
    QString toolName;
    QString toolCallId;
    qint64 executionTimeMs = 0;
    
    static ToolResult ok(const QVariant &result, const QString &toolName, const QString &callId) {
        ToolResult r;
        r.success = true;
        r.data = result;
        r.toolName = toolName;
        r.toolCallId = callId;
        return r;
    }
    
    static ToolResult error(const QString &msg, const QString &toolName, const QString &callId) {
        ToolResult r;
        r.success = false;
        r.errorMessage = msg;
        r.toolName = toolName;
        r.toolCallId = callId;
        return r;
    }
};

/**
 * @brief Definición de una AI Tool.
 * 
 * Cada tool debe declarar explícitamente su comportamiento, inputs y riesgos.
 */
class AITool : public QObject {
    Q_OBJECT

public:
    explicit AITool(QObject *parent = nullptr);
    ~AITool() override = default;

    /**
     * @brief Nombre único de la tool.
     */
    virtual QString name() const = 0;

    /**
     * @brief Descripción legible para humanos y LLMs.
     */
    virtual QString description() const = 0;

    /**
     * @brief Esquema de parámetros de entrada.
     */
    virtual std::vector<ToolParameterSchema> inputSchema() const = 0;

    /**
     * @brief Esquema del resultado esperado.
     */
    virtual std::vector<ToolParameterSchema> outputSchema() const = 0;

    /**
     * @brief Nivel de riesgo de la operación.
     */
    virtual ToolRiskLevel riskLevel() const = 0;

    /**
     * @brief Permisos requeridos para ejecutar esta tool.
     */
    virtual QStringList requiredPermissions() const = 0;

    /**
     * @brief Si requiere confirmación explícita del usuario.
     */
    virtual bool requiresConfirmation() const = 0;

    /**
     * @brief Ejecuta la tool con los parámetros dados.
     * @param params Parámetros validados
     * @param context Contexto adicional (project, timeline, etc.)
     * @return Resultado de la ejecución
     */
    virtual ToolResult execute(const QVariantMap &params, const QVariantMap &context) = 0;

    /**
     * @brief Valida los parámetros de entrada contra el schema.
     * @param params Parámetros a validar
     * @return Lista de errores de validación (vacía si válido)
     */
    QStringList validateParams(const QVariantMap &params) const;

    /**
     * @brief Convierte el schema a JSON para enviar al LLM.
     */
    QJsonObject toJson() const;

protected:
    template<typename T>
    T getParam(const QVariantMap &params, const QString &name, const T &defaultVal) const {
        if (!params.contains(name)) return defaultVal;
        return params[name].value<T>();
    }
    
    template<typename T>
    std::optional<T> getOptionalParam(const QVariantMap &params, const QString &name) const {
        if (!params.contains(name)) return std::nullopt;
        return params[name].value<T>();
    }
};

/**
 * @brief Sistema de gestión de AI Tools.
 * 
 * CARACTERÍSTICAS DE SEGURIDAD:
 * - Validación estricta de inputs
 * - Clasificación por nivel de riesgo
 * - Control de permisos
 * - Confirmación requerida para operaciones sensibles
 * - Logging estructurado
 * - Prevención de inyección de comandos
 * - Límites en respuestas del LLM
 */
class AIToolRegistry : public QObject {
    Q_OBJECT

public:
    explicit AIToolRegistry(QObject *parent = nullptr);
    ~AIToolRegistry() override = default;

    /**
     * @brief Registra una tool en el sistema.
     */
    void registerTool(std::shared_ptr<AITool> tool);

    /**
     * @brief Desregistra una tool.
     */
    void unregisterTool(const QString &toolName);

    /**
     * @brief Obtiene una tool por nombre.
     */
    std::shared_ptr<AITool> getTool(const QString &toolName) const;

    /**
     * @brief Lista todas las tools registradas.
     */
    QStringList listTools() const;

    /**
     * @brief Ejecuta una tool con validación completa.
     * @param toolName Nombre de la tool
     * @param params Parámetros de entrada
     * @param context Contexto de ejecución
     * @param requireConfirmation Si true, bloquea tools que requieren confirmación
     * @return Resultado de la ejecución
     */
    ToolResult executeTool(
        const QString &toolName,
        const QVariantMap &params,
        const QVariantMap &context = {},
        bool requireConfirmation = false
    );

    /**
     * @brief Valida un payload de tool call desde un LLM.
     * @param payload JSON del LLM
     * @return Errores de validación
     */
    QStringList validateLLMPayload(const QJsonObject &payload) const;

    /**
     * @brief Genera descripción JSON de todas las tools para el LLM.
     */
    QJsonDocument generateToolsDescription() const;

    /**
     * @brief Habilita/deshabilita una tool.
     */
    void setToolEnabled(const QString &toolName, bool enabled);

    /**
     * @brief Verifica si una tool está habilitada.
     */
    bool isToolEnabled(const QString &toolName) const;

Q_SIGNALS:
    void toolExecuted(const QString &toolName, const ToolResult &result);
    void toolValidationFailed(const QString &toolName, const QStringList &errors);
    void permissionDenied(const QString &toolName, const QString &missingPermission);
    void confirmationRequired(const QString &toolName, const QVariantMap &params);

private:
    QMap<QString, std::shared_ptr<AITool>> m_tools;
    QSet<QString> m_enabledTools;
    mutable QMutex m_mutex;
};

} // namespace ccos::core
