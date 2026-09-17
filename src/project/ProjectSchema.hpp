#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>
#include <unordered_map>
#include <functional>

namespace ccos {

// ============================================================================
// FASE 7: PROJECT FORMAT - Schema Versioning & Migration
// ============================================================================

/**
 * @brief Versiones del schema del proyecto
 */
enum class ProjectSchemaVersion {
    UNKNOWN = 0,
    V1_0_0 = 1,      // Versión inicial
    V1_1_0 = 2,      // Soporte para múltiples pistas de audio
    V1_2_0 = 3,      // Soporte para efectos y transiciones
    V1_3_0 = 4,      // Soporte para subtítulos
    V1_4_0 = 5,      // Soporte para keyframes
    CURRENT = V1_4_0
};

/**
 * @brief Resultado de validación de schema
 */
struct SchemaValidationResult {
    bool isValid{false};
    std::string version;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> unknownFields;
    
    std::string toString() const;
};

/**
 * @brief Información de migración
 */
struct MigrationInfo {
    ProjectSchemaVersion fromVersion;
    ProjectSchemaVersion toVersion;
    std::string description;
    bool isBreaking{false};
    std::function<bool(const std::string& input, std::string& output)> migrateFunction;
};

/**
 * @brief Gestor de schemas y migraciones
 * 
 * Responsabilidades:
 * - Validar versiones de schema
 * - Ejecutar migraciones entre versiones
 * - Detectar corrupción de datos
 * - Manejar campos desconocidos
 * - Preservar IDs estables
 */
class ProjectSchemaManager {
public:
    static ProjectSchemaManager& instance();
    
    /**
     * @brief Obtiene la versión actual del schema
     */
    ProjectSchemaVersion getCurrentVersion() const;
    
    /**
     * @brief Convierte string a versión
     */
    std::optional<ProjectSchemaVersion> parseVersion(const std::string& versionStr) const;
    
    /**
     * @brief Convierte versión a string
     */
    std::string versionToString(ProjectSchemaVersion version) const;
    
    /**
     * @brief Valida un proyecto JSON contra el schema
     */
    SchemaValidationResult validate(const std::string& jsonContent) const;
    
    /**
     * @brief Detecta la versión de un proyecto
     */
    std::optional<ProjectSchemaVersion> detectVersion(const std::string& jsonContent) const;
    
    /**
     * @brief Migra un proyecto a la versión actual
     * @return true si la migración fue exitosa
     */
    bool migrateToCurrent(std::string& jsonContent);
    
    /**
     * @brief Migra un proyecto a una versión específica
     */
    bool migrateTo(std::string& jsonContent, ProjectSchemaVersion targetVersion);
    
    /**
     * @brief Registra una función de migración
     */
    void registerMigration(MigrationInfo info);
    
    /**
     * @brief Verifica si un proyecto está corrupto
     */
    bool isCorrupted(const std::string& jsonContent) const;
    
    /**
     * @brief Intenta reparar un proyecto corrupto
     */
    std::optional<std::string> attemptRepair(const std::string& jsonContent) const;
    
    /**
     * @brief Obtiene información de compatibilidad
     */
    struct CompatibilityInfo {
        bool isCompatible{false};
        bool requiresMigration{false};
        ProjectSchemaVersion projectVersion{ProjectSchemaVersion::UNKNOWN};
        ProjectSchemaVersion currentVersion{ProjectSchemaVersion::CURRENT};
        std::string message;
    };
    
    CompatibilityInfo checkCompatibility(const std::string& jsonContent) const;
    
private:
    ProjectSchemaManager();
    ~ProjectSchemaManager();
    
    void initializeDefaultMigrations();
    std::vector<MigrationInfo> migrations_;
    ProjectSchemaVersion currentVersion_{ProjectSchemaVersion::CURRENT};
};

/**
 * @brief Serializador de proyectos con validación de schema
 */
class ProjectSerializer {
public:
    struct SerializeOptions {
        bool prettyPrint{true};
        bool includeMetadata{true};
        bool validateBeforeSerialize{true};
        ProjectSchemaVersion targetVersion{ProjectSchemaVersion::CURRENT};
    };
    
    /**
     * @brief Serializa un proyecto a JSON
     */
    static std::string serialize(const std::string& projectData, 
                                 const SerializeOptions& options = SerializeOptions());
    
    /**
     * @brief Deserializa JSON a proyecto
     */
    static std::pair<std::string, SchemaValidationResult> deserialize(const std::string& jsonContent);
    
    /**
     * @brief Guarda un proyecto en disco con validación
     */
    static bool saveToFile(const std::string& filePath, 
                          const std::string& projectData,
                          const SerializeOptions& options = SerializeOptions());
    
    /**
     * @brief Carga un proyecto desde disco con migración automática
     */
    static std::pair<std::string, SchemaValidationResult> loadFromFile(const std::string& filePath);
    
    /**
     * @brief Crea un backup antes de modificar
     */
    static std::string createBackup(const std::string& filePath);
    
    /**
     * @brief Restaura desde backup
     */
    static bool restoreFromBackup(const std::string& backupPath, const std::string& targetPath);
};

/**
 * @brief Validador de integridad de proyectos
 */
class ProjectValidator {
public:
    struct ValidationReport {
        bool isValid{false};
        bool hasWarnings{false};
        bool hasErrors{false};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> fixedIssues;
        
        std::string summary() const;
    };
    
    /**
     * @brief Valida la estructura completa del proyecto
     */
    static ValidationReport validate(const std::string& projectData);
    
    /**
     * @brief Valida IDs únicos
     */
    static bool validateUniqueIds(const std::string& projectData);
    
    /**
     * @brief Valida referencias cruzadas
     */
    static bool validateCrossReferences(const std::string& projectData);
    
    /**
     * @brief Valida tiempos y duración
     */
    static bool validateTimings(const std::string& projectData);
    
    /**
     * @brief Valida assets existentes
     */
    static bool validateAssets(const std::string& projectData);
    
    /**
     * @brief Corrige problemas comunes automáticamente
     */
    static std::pair<std::string, ValidationReport> autoFix(const std::string& projectData);
};

/**
 * @brief Gestor de fixtures para tests
 */
class ProjectFixtures {
public:
    /**
     * @brief Crea un proyecto válido mínimo
     */
    static std::string createMinimalValidProject();
    
    /**
     * @brief Crea un proyecto válido completo
     */
    static std::string createFullValidProject();
    
    /**
     * @brief Crea un proyecto con versión anterior
     */
    static std::string createLegacyProject(ProjectSchemaVersion version);
    
    /**
     * @brief Crea un proyecto corrupto para tests
     */
    static std::string createCorruptedProject(const std::string& corruptionType);
    
    /**
     * @brief Crea un proyecto con campos desconocidos
     */
    static std::string createProjectWithUnknownFields();
    
    /**
     * @brief Crea un proyecto grande para tests de performance
     */
    static std::string createLargeProject(int clipCount);
    
    /**
     * @brief Crea un proyecto vacío
     */
    static std::string createEmptyProject();
};

} // namespace ccos
