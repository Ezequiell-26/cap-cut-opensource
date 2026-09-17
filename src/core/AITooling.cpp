#include "core/AITooling.hpp"
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QDateTime>

Q_LOGGING_CATEGORY(ccos_core_aitool, "ccos.core.aitool")

namespace ccos::core {

// ============================================================================
// ToolParameterSchema Implementation
// ============================================================================

bool ToolParameterSchema::validate(const QVariant &value) const {
    // Verificar tipo
    if (type == "string") {
        if (!value.canConvert<QString>()) return false;
        QString str = value.toString();
        
        if (minLength >= 0 && str.length() < minLength) return false;
        if (maxLength >= 0 && str.length() > maxLength) return false;
        if (!pattern.isEmpty()) {
            QRegularExpression re(pattern);
            if (!re.match(str).hasMatch()) return false;
        }
    } else if (type == "number") {
        if (!value.canConvert<double>()) return false;
        double num = value.toDouble();
        if (num < minimum || num > maximum) return false;
    } else if (type == "boolean") {
        if (!value.canConvert<bool>()) return false;
    } else if (type == "array") {
        if (!value.canConvert<QVariantList>()) return false;
    } else if (type == "object") {
        if (!value.canConvert<QVariantMap>()) return false;
    }
    
    // Verificar enum si existe
    if (!enumValues.isEmpty()) {
        if (!enumValues.contains(value.toString())) return false;
    }
    
    return true;
}

QString ToolParameterSchema::validationError(const QVariant &value) const {
    if (type == "string") {
        if (!value.canConvert<QString>()) {
            return QString("Parameter '%1' must be a string").arg(name);
        }
        QString str = value.toString();
        if (minLength >= 0 && str.length() < minLength) {
            return QString("Parameter '%1' is too short (min %2 chars)").arg(name).arg(minLength);
        }
        if (maxLength >= 0 && str.length() > maxLength) {
            return QString("Parameter '%1' is too long (max %2 chars)").arg(name).arg(maxLength);
        }
        if (!pattern.isEmpty()) {
            QRegularExpression re(pattern);
            if (!re.match(str).hasMatch()) {
                return QString("Parameter '%1' does not match required pattern").arg(name);
            }
        }
    } else if (type == "number") {
        if (!value.canConvert<double>()) {
            return QString("Parameter '%1' must be a number").arg(name);
        }
        double num = value.toDouble();
        if (num < minimum) {
            return QString("Parameter '%1' is below minimum (%2)").arg(name).arg(minimum);
        }
        if (num > maximum) {
            return QString("Parameter '%1' is above maximum (%2)").arg(name).arg(maximum);
        }
    } else if (type == "boolean") {
        if (!value.canConvert<bool>()) {
            return QString("Parameter '%1' must be a boolean").arg(name);
        }
    } else if (type == "array") {
        if (!value.canConvert<QVariantList>()) {
            return QString("Parameter '%1' must be an array").arg(name);
        }
    } else if (type == "object") {
        if (!value.canConvert<QVariantMap>()) {
            return QString("Parameter '%1' must be an object").arg(name);
        }
    }
    
    if (!enumValues.isEmpty() && !enumValues.contains(value.toString())) {
        return QString("Parameter '%1' must be one of: %2")
            .arg(name)
            .arg(enumValues.join(", "));
    }
    
    return {};
}

// ============================================================================
// AITool Implementation
// ============================================================================

AITool::AITool(QObject *parent) : QObject(parent) {}

QStringList AITool::validateParams(const QVariantMap &params) const {
    QStringList errors;
    
    auto schema = inputSchema();
    
    // Verificar parámetros requeridos
    for (const auto &param : schema) {
        if (param.required && !params.contains(param.name)) {
            errors.append(QString("Missing required parameter: %1").arg(param.name));
        }
    }
    
    // Validar cada parámetro proporcionado
    for (auto it = params.begin(); it != params.end(); ++it) {
        const QString &name = it.key();
        const QVariant &value = it.value();
        
        // Buscar schema para este parámetro
        auto found = std::find_if(schema.begin(), schema.end(),
            [&name](const ToolParameterSchema &s) { return s.name == name; });
        
        if (found != schema.end()) {
            if (!found->validate(value)) {
                errors.append(found->validationError(value));
            }
        } else {
            // Parámetro desconocido - podría ser error o warning según política
            qCWarning(ccos_core_aitool) << "Unknown parameter:" << name 
                                        << "for tool:" << name();
        }
    }
    
    return errors;
}

QJsonObject AITool::toJson() const {
    QJsonObject obj;
    obj["name"] = name();
    obj["description"] = description();
    obj["riskLevel"] = static_cast<int>(riskLevel());
    obj["requiresConfirmation"] = requiresConfirmation();
    obj["requiredPermissions"] = QJsonArray::fromStringList(requiredPermissions());
    
    // Input schema
    QJsonObject inputSchemaObj;
    QJsonObject properties;
    QJsonArray required;
    
    for (const auto &param : inputSchema()) {
        QJsonObject paramObj;
        paramObj["type"] = param.type;
        paramObj["description"] = param.description;
        
        if (param.required) {
            required.append(param.name);
        }
        
        if (param.minLength >= 0) paramObj["minLength"] = param.minLength;
        if (param.maxLength >= 0) paramObj["maxLength"] = param.maxLength;
        if (param.minimum != std::numeric_limits<double>::lowest()) {
            paramObj["minimum"] = param.minimum;
        }
        if (param.maximum != std::numeric_limits<double>::max()) {
            paramObj["maximum"] = param.maximum;
        }
        if (!param.pattern.isEmpty()) paramObj["pattern"] = param.pattern;
        if (!param.enumValues.isEmpty()) {
            paramObj["enum"] = QJsonArray::fromStringList(param.enumValues);
        }
        
        properties[param.name] = paramObj;
    }
    
    inputSchemaObj["type"] = "object";
    inputSchemaObj["properties"] = properties;
    if (!required.isEmpty()) {
        inputSchemaObj["required"] = required;
    }
    
    obj["inputSchema"] = inputSchemaObj;
    
    return obj;
}

// ============================================================================
// AIToolRegistry Implementation
// ============================================================================

AIToolRegistry::AIToolRegistry(QObject *parent) : QObject(parent) {}

void AIToolRegistry::registerTool(std::shared_ptr<AITool> tool) {
    if (!tool) {
        qCWarning(ccos_core_aitool) << "Attempted to register null tool";
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QString toolName = tool->name();
    if (m_tools.contains(toolName)) {
        qCWarning(ccos_core_aitool) << "Tool already registered:" << toolName;
        return;
    }
    
    m_tools[toolName] = tool;
    m_enabledTools.insert(toolName);
    
    qCInfo(ccos_core_aitool) << "Tool registered:" << toolName 
                             << "risk:" << static_cast<int>(tool->riskLevel());
}

void AIToolRegistry::unregisterTool(const QString &toolName) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_tools.contains(toolName)) {
        qCWarning(ccos_core_aitool) << "Tool not found:" << toolName;
        return;
    }
    
    m_tools.remove(toolName);
    m_enabledTools.remove(toolName);
    
    qCInfo(ccos_core_aitool) << "Tool unregistered:" << toolName;
}

std::shared_ptr<AITool> AIToolRegistry::getTool(const QString &toolName) const {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_tools.find(toolName);
    if (it == m_tools.end()) {
        return nullptr;
    }
    
    return *it;
}

QStringList AIToolRegistry::listTools() const {
    QMutexLocker locker(&m_mutex);
    return m_tools.keys();
}

ToolResult AIToolRegistry::executeTool(
    const QString &toolName,
    const QVariantMap &params,
    const QVariantMap &context,
    bool requireConfirmation
) {
    QMutexLocker locker(&m_mutex);
    
    auto it = m_tools.find(toolName);
    if (it == m_tools.end()) {
        qCWarning(ccos_core_aitool) << "Tool not found:" << toolName;
        return ToolResult::error("Tool not found: " + toolName, toolName, QUuid::createUuid().toString());
    }
    
    auto tool = *it;
    
    // Verificar si está habilitada
    if (!m_enabledTools.contains(toolName)) {
        qCWarning(ccos_core_aitool) << "Tool disabled:" << toolName;
        return ToolResult::error("Tool is disabled: " + toolName, toolName, QUuid::createUuid().toString());
    }
    
    // Validar parámetros
    QStringList validationErrors = tool->validateParams(params);
    if (!validationErrors.isEmpty()) {
        qCWarning(ccos_core_aitool) << "Tool validation failed:" << toolName 
                                    << "errors:" << validationErrors;
        emit toolValidationFailed(toolName, validationErrors);
        return ToolResult::error(validationErrors.join("; "), toolName, QUuid::createUuid().toString());
    }
    
    // Verificar confirmación requerida
    if (requireConfirmation && tool->requiresConfirmation()) {
        qCInfo(ccos_core_aitool) << "Tool requires confirmation:" << toolName;
        emit confirmationRequired(toolName, params);
        return ToolResult::error("Tool requires user confirmation", toolName, QUuid::createUuid().toString());
    }
    
    // Ejecutar tool con timing
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    
    try {
        ToolResult result = tool->execute(params, context);
        result.executionTimeMs = QDateTime::currentMSecsSinceEpoch() - startTime;
        
        if (result.success) {
            qCInfo(ccos_core_aitool) << "Tool executed successfully:" << toolName 
                                     << "time:" << result.executionTimeMs << "ms";
        } else {
            qCWarning(ccos_core_aitool) << "Tool execution failed:" << toolName 
                                        << "error:" << result.errorMessage;
        }
        
        emit toolExecuted(toolName, result);
        return result;
    } catch (const std::exception &e) {
        QString errorMsg = QString::fromUtf8(e.what());
        qCWarning(ccos_core_aitool) << "Tool threw exception:" << toolName 
                                    << "error:" << errorMsg;
        return ToolResult::error(errorMsg, toolName, QUuid::createUuid().toString());
    } catch (...) {
        qCWarning(ccos_core_aitool) << "Tool threw unknown exception:" << toolName;
        return ToolResult::error("Unknown exception", toolName, QUuid::createUuid().toString());
    }
}

QStringList AIToolRegistry::validateLLMPayload(const QJsonObject &payload) const {
    QStringList errors;
    
    // Verificar estructura básica
    if (!payload.contains("name") || !payload["name"].isString()) {
        errors.append("Missing or invalid 'name' field");
    }
    
    if (!payload.contains("arguments") || !payload["arguments"].isObject()) {
        errors.append("Missing or invalid 'arguments' field");
    }
    
    // Si tiene nombre válido, verificar que existe
    if (payload.contains("name") && payload["name"].isString()) {
        QString toolName = payload["name"].toString();
        
        QMutexLocker locker(&m_mutex);
        if (!m_tools.contains(toolName)) {
            errors.append("Unknown tool: " + toolName);
        }
    }
    
    return errors;
}

QJsonDocument AIToolRegistry::generateToolsDescription() const {
    QMutexLocker locker(&m_mutex);
    
    QJsonArray toolsArray;
    
    for (auto it = m_tools.begin(); it != m_tools.end(); ++it) {
        if (m_enabledTools.contains(it.key())) {
            toolsArray.append(it.value()->toJson());
        }
    }
    
    QJsonObject root;
    root["tools"] = toolsArray;
    
    return QJsonDocument(root);
}

void AIToolRegistry::setToolEnabled(const QString &toolName, bool enabled) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_tools.contains(toolName)) {
        qCWarning(ccos_core_aitool) << "Cannot enable/disable unknown tool:" << toolName;
        return;
    }
    
    if (enabled) {
        m_enabledTools.insert(toolName);
        qCInfo(ccos_core_aitool) << "Tool enabled:" << toolName;
    } else {
        m_enabledTools.remove(toolName);
        qCInfo(ccos_core_aitool) << "Tool disabled:" << toolName;
    }
}

bool AIToolRegistry::isToolEnabled(const QString &toolName) const {
    QMutexLocker locker(&m_mutex);
    return m_enabledTools.contains(toolName);
}

} // namespace ccos::core
