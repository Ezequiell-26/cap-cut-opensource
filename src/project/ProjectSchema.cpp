#include "ProjectSchema.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <regex>
#include <chrono>
#include <iomanip>

// Nota: En implementación real, usar nlohmann/json o similar
// Aquí implementamos lógica de validación básica para demostración

namespace ccos {

// ============================================================================
// SchemaValidationResult Implementation
// ============================================================================

std::string SchemaValidationResult::toString() const {
    std::ostringstream oss;
    oss << "Schema Validation Result:\n";
    oss << "  Valid: " << (isValid ? "YES" : "NO") << "\n";
    oss << "  Version: " << version << "\n";
    
    if (!errors.empty()) {
        oss << "  Errors (" << errors.size() << "):\n";
        for (const auto& err : errors) {
            oss << "    - " << err << "\n";
        }
    }
    
    if (!warnings.empty()) {
        oss << "  Warnings (" << warnings.size() << "):\n";
        for (const auto& warn : warnings) {
            oss << "    - " << warn << "\n";
        }
    }
    
    if (!unknownFields.empty()) {
        oss << "  Unknown Fields (" << unknownFields.size() << "):\n";
        for (const auto& field : unknownFields) {
            oss << "    - " << field << "\n";
        }
    }
    
    return oss.str();
}

// ============================================================================
// ProjectValidator::ValidationReport Implementation
// ============================================================================

std::string ProjectValidator::ValidationReport::summary() const {
    std::ostringstream oss;
    oss << "Validation Summary: ";
    
    if (isValid) {
        oss << "PASSED";
    } else {
        oss << "FAILED";
    }
    
    oss << " | Errors: " << errors.size();
    oss << " | Warnings: " << warnings.size();
    
    if (!fixedIssues.empty()) {
        oss << " | Fixed: " << fixedIssues.size();
    }
    
    return oss.str();
}

// ============================================================================
// ProjectSchemaManager Implementation
// ============================================================================

ProjectSchemaManager& ProjectSchemaManager::instance() {
    static ProjectSchemaManager instance;
    return instance;
}

ProjectSchemaManager::ProjectSchemaManager() {
    initializeDefaultMigrations();
}

ProjectSchemaManager::~ProjectSchemaManager() = default;

void ProjectSchemaManager::initializeDefaultMigrations() {
    // Migración V1.0.0 -> V1.1.0: Soporte para múltiples pistas de audio
    MigrationInfo v1_0_0_to_v1_1_0;
    v1_0_0_to_v1_1_0.fromVersion = ProjectSchemaVersion::V1_0_0;
    v1_0_0_to_v1_1_0.toVersion = ProjectSchemaVersion::V1_1_0;
    v1_0_0_to_v1_1_0.description = "Agregar soporte para múltiples pistas de audio";
    v1_0_0_to_v1_1_0.isBreaking = false;
    v1_0_0_to_v1_1_0.migrateFunction = [](const std::string& input, std::string& output) -> bool {
        // Implementación simplificada - en producción usar parser JSON real
        output = input;
        // Agregar campo audioTracks si no existe
        if (output.find("\"audioTracks\"") == std::string::npos) {
            // Insertar audioTracks vacío después de videoTracks
            size_t pos = output.find("\"videoTracks\"");
            if (pos != std::string::npos) {
                // Buscar el cierre del array de videoTracks
                int bracketCount = 0;
                size_t endPos = pos;
                while (endPos < output.size()) {
                    if (output[endPos] == '[') bracketCount++;
                    if (output[endPos] == ']') bracketCount--;
                    if (bracketCount == 0) break;
                    endPos++;
                }
                if (endPos < output.size()) {
                    output.insert(endPos + 1, ",\n    \"audioTracks\": []");
                    return true;
                }
            }
        }
        return true;
    };
    migrations_.push_back(v1_0_0_to_v1_1_0);
    
    // Migración V1.1.0 -> V1.2.0: Soporte para efectos y transiciones
    MigrationInfo v1_1_0_to_v1_2_0;
    v1_1_0_to_v1_2_0.fromVersion = ProjectSchemaVersion::V1_1_0;
    v1_1_0_to_v1_2_0.toVersion = ProjectSchemaVersion::V1_2_0;
    v1_1_0_to_v1_2_0.description = "Agregar soporte para efectos y transiciones";
    v1_1_0_to_v1_2_0.isBreaking = false;
    v1_1_0_to_v1_2_0.migrateFunction = [](const std::string& input, std::string& output) -> bool {
        output = input;
        // Agregar campos effects y transitions si no existen
        if (output.find("\"effects\"") == std::string::npos) {
            output.insert(output.size() - 2, ",\n    \"effects\": []");
        }
        if (output.find("\"transitions\"") == std::string::npos) {
            output.insert(output.size() - 2, ",\n    \"transitions\": []");
        }
        return true;
    };
    migrations_.push_back(v1_1_0_to_v1_2_0);
    
    // Migración V1.2.0 -> V1.3.0: Soporte para subtítulos
    MigrationInfo v1_2_0_to_v1_3_0;
    v1_2_0_to_v1_3_0.fromVersion = ProjectSchemaVersion::V1_2_0;
    v1_2_0_to_v1_3_0.toVersion = ProjectSchemaVersion::V1_3_0;
    v1_2_0_to_v1_3_0.description = "Agregar soporte para subtítulos";
    v1_2_0_to_v1_3_0.isBreaking = false;
    v1_2_0_to_v1_3_0.migrateFunction = [](const std::string& input, std::string& output) -> bool {
        output = input;
        if (output.find("\"subtitles\"") == std::string::npos) {
            output.insert(output.size() - 2, ",\n    \"subtitles\": []");
        }
        return true;
    };
    migrations_.push_back(v1_2_0_to_v1_3_0);
    
    // Migración V1.3.0 -> V1.4.0: Soporte para keyframes
    MigrationInfo v1_3_0_to_v1_4_0;
    v1_3_0_to_v1_4_0.fromVersion = ProjectSchemaVersion::V1_3_0;
    v1_3_0_to_v1_4_0.toVersion = ProjectSchemaVersion::V1_4_0;
    v1_3_0_to_v1_4_0.description = "Agregar soporte para keyframes en efectos";
    v1_3_0_to_v1_4_0.isBreaking = false;
    v1_3_0_to_v1_4_0.migrateFunction = [](const std::string& input, std::string& output) -> bool {
        output = input;
        // Los proyectos antiguos no tienen keyframes, es compatible hacia atrás
        return true;
    };
    migrations_.push_back(v1_3_0_to_v1_4_0);
}

ProjectSchemaVersion ProjectSchemaManager::getCurrentVersion() const {
    return currentVersion_;
}

std::optional<ProjectSchemaVersion> ProjectSchemaManager::parseVersion(const std::string& versionStr) const {
    if (versionStr == "1.0.0" || versionStr == "1.0") return ProjectSchemaVersion::V1_0_0;
    if (versionStr == "1.1.0" || versionStr == "1.1") return ProjectSchemaVersion::V1_1_0;
    if (versionStr == "1.2.0" || versionStr == "1.2") return ProjectSchemaVersion::V1_2_0;
    if (versionStr == "1.3.0" || versionStr == "1.3") return ProjectSchemaVersion::V1_3_0;
    if (versionStr == "1.4.0" || versionStr == "1.4") return ProjectSchemaVersion::V1_4_0;
    return std::nullopt;
}

std::string ProjectSchemaManager::versionToString(ProjectSchemaVersion version) const {
    switch (version) {
        case ProjectSchemaVersion::V1_0_0: return "1.0.0";
        case ProjectSchemaVersion::V1_1_0: return "1.1.0";
        case ProjectSchemaVersion::V1_2_0: return "1.2.0";
        case ProjectSchemaVersion::V1_3_0: return "1.3.0";
        case ProjectSchemaVersion::V1_4_0: return "1.4.0";
        default: return "unknown";
    }
}

std::optional<ProjectSchemaVersion> ProjectSchemaManager::detectVersion(const std::string& jsonContent) const {
    // Detectar versión buscando el campo schemaVersion
    std::regex versionRegex(R"("schemaVersion"\s*:\s*"([^"]+)")");
    std::smatch match;
    
    if (std::regex_search(jsonContent, match, versionRegex) && match.size() > 1) {
        return parseVersion(match[1].str());
    }
    
    // Si no hay schemaVersion, intentar detectar por características
    if (jsonContent.find("\"keyframes\"") != std::string::npos) {
        return ProjectSchemaVersion::V1_4_0;
    }
    if (jsonContent.find("\"subtitles\"") != std::string::npos) {
        return ProjectSchemaVersion::V1_3_0;
    }
    if (jsonContent.find("\"effects\"") != std::string::npos || 
        jsonContent.find("\"transitions\"") != std::string::npos) {
        return ProjectSchemaVersion::V1_2_0;
    }
    if (jsonContent.find("\"audioTracks\"") != std::string::npos) {
        return ProjectSchemaVersion::V1_1_0;
    }
    
    // Asumir V1.0.0 como fallback
    return ProjectSchemaVersion::V1_0_0;
}

SchemaValidationResult ProjectSchemaManager::validate(const std::string& jsonContent) const {
    SchemaValidationResult result;
    
    // Verificar que sea JSON válido (básico)
    if (jsonContent.empty()) {
        result.errors.push_back("Contenido vacío");
        return result;
    }
    
    // Detectar versión
    auto detectedVersion = detectVersion(jsonContent);
    if (!detectedVersion.has_value()) {
        result.errors.push_back("No se pudo detectar la versión del schema");
        return result;
    }
    
    result.version = versionToString(detectedVersion.value());
    
    // Campos requeridos básicos
    std::vector<std::string> requiredFields = {"schemaVersion", "id", "name", "createdAt"};
    for (const auto& field : requiredFields) {
        if (jsonContent.find("\"" + field + "\"") == std::string::npos) {
            result.errors.push_back("Campo requerido faltante: " + field);
        }
    }
    
    // Validar estructura básica
    if (jsonContent.front() != '{' || jsonContent.back() != '}') {
        result.errors.push_back("JSON malformado: debe comenzar con { y terminar con }");
    }
    
    // Verificar brackets balanceados
    int braceCount = 0;
    int bracketCount = 0;
    for (char c : jsonContent) {
        if (c == '{') braceCount++;
        if (c == '}') braceCount--;
        if (c == '[') bracketCount++;
        if (c == ']') bracketCount--;
        
        if (braceCount < 0 || bracketCount < 0) {
            result.errors.push_back("Brackets desbalanceados");
            break;
        }
    }
    
    if (braceCount != 0) {
        result.errors.push_back("Llaves desbalanceadas");
    }
    if (bracketCount != 0) {
        result.errors.push_back("Corchetes desbalanceados");
    }
    
    // Detectar campos desconocidos (warning)
    std::set<std::string> knownFields = {
        "schemaVersion", "id", "name", "description", "createdAt", "updatedAt",
        "videoTracks", "audioTracks", "clips", "assets", "effects", "transitions",
        "subtitles", "markers", "settings", "metadata", "keyframes"
    };
    
    std::regex fieldRegex(R"("([a-zA-Z_][a-zA-Z0-9_]*)"\s*:)");
    auto fieldsBegin = std::sregex_iterator(jsonContent.begin(), jsonContent.end(), fieldRegex);
    auto fieldsEnd = std::sregex_iterator();
    
    for (auto it = fieldsBegin; it != fieldsEnd; ++it) {
        std::string fieldName = (*it)[1].str();
        if (knownFields.find(fieldName) == knownFields.end()) {
            result.unknownFields.push_back(fieldName);
            result.warnings.push_back("Campo desconocido: " + fieldName);
        }
    }
    
    result.isValid = result.errors.empty();
    return result;
}

bool ProjectSchemaManager::migrateToCurrent(std::string& jsonContent) {
    auto currentDetected = detectVersion(jsonContent);
    if (!currentDetected.has_value()) {
        return false;
    }
    
    return migrateTo(jsonContent, ProjectSchemaVersion::CURRENT);
}

bool ProjectSchemaManager::migrateTo(std::string& jsonContent, ProjectSchemaVersion targetVersion) {
    auto currentDetected = detectVersion(jsonContent);
    if (!currentDetected.has_value()) {
        return false;
    }
    
    ProjectSchemaVersion current = currentDetected.value();
    
    if (current == targetVersion) {
        return true; // Ya está en la versión objetivo
    }
    
    if (current > targetVersion) {
        // No soportamos downgrade automáticamente
        return false;
    }
    
    // Aplicar migraciones secuenciales
    std::string workingContent = jsonContent;
    ProjectSchemaVersion workingVersion = current;
    
    while (workingVersion < targetVersion) {
        bool migrationFound = false;
        
        for (const auto& migration : migrations_) {
            if (migration.fromVersion == workingVersion && migration.toVersion <= targetVersion) {
                std::string nextContent;
                if (migration.migrateFunction(workingContent, nextContent)) {
                    workingContent = nextContent;
                    workingVersion = migration.toVersion;
                    migrationFound = true;
                    break;
                }
            }
        }
        
        if (!migrationFound) {
            // No se encontró migración para esta versión
            return false;
        }
    }
    
    jsonContent = workingContent;
    return true;
}

void ProjectSchemaManager::registerMigration(MigrationInfo info) {
    migrations_.push_back(info);
}

bool ProjectSchemaManager::isCorrupted(const std::string& jsonContent) const {
    if (jsonContent.empty()) return true;
    
    // Verificaciones básicas de corrupción
    int braceCount = 0;
    int bracketCount = 0;
    
    for (char c : jsonContent) {
        if (c == '{') braceCount++;
        if (c == '}') braceCount--;
        if (c == '[') bracketCount++;
        if (c == ']') bracketCount--;
        
        if (braceCount < 0 || bracketCount < 0) return true;
    }
    
    return (braceCount != 0 || bracketCount != 0);
}

std::optional<std::string> ProjectSchemaManager::attemptRepair(const std::string& jsonContent) const {
    if (!isCorrupted(jsonContent)) {
        return jsonContent; // No necesita reparación
    }
    
    std::string repaired = jsonContent;
    
    // Intentar reparar brackets desbalanceados
    int braceCount = 0;
    int bracketCount = 0;
    
    for (char c : repaired) {
        if (c == '{') braceCount++;
        if (c == '}') braceCount--;
        if (c == '[') bracketCount++;
        if (c == ']') bracketCount--;
    }
    
    // Agregar brackets faltantes al final
    while (braceCount < 0) {
        repaired += '}';
        braceCount++;
    }
    while (bracketCount < 0) {
        repaired += ']';
        bracketCount++;
    }
    
    // Eliminar brackets sobrantes del final
    while (braceCount > 0 && !repaired.empty() && repaired.back() == '{') {
        repaired.pop_back();
        braceCount--;
    }
    while (bracketCount > 0 && !repaired.empty() && repaired.back() == '[') {
        repaired.pop_back();
        bracketCount--;
    }
    
    // Verificar si la reparación fue exitosa
    if (!isCorrupted(repaired)) {
        return repaired;
    }
    
    return std::nullopt; // No se pudo reparar
}

ProjectSchemaManager::CompatibilityInfo ProjectSchemaManager::checkCompatibility(const std::string& jsonContent) const {
    CompatibilityInfo info;
    info.currentVersion = currentVersion_;
    
    auto detectedVersion = detectVersion(jsonContent);
    if (!detectedVersion.has_value()) {
        info.message = "No se pudo detectar la versión del proyecto";
        return info;
    }
    
    info.projectVersion = detectedVersion.value();
    
    if (info.projectVersion == info.currentVersion) {
        info.isCompatible = true;
        info.requiresMigration = false;
        info.message = "Proyecto compatible";
    } else if (info.projectVersion < info.currentVersion) {
        info.isCompatible = true;
        info.requiresMigration = true;
        info.message = "Proyecto requiere migración de " + 
                       versionToString(info.projectVersion) + " a " + 
                       versionToString(info.currentVersion);
    } else {
        info.isCompatible = false;
        info.requiresMigration = false;
        info.message = "Proyecto creado con versión futura incompatible";
    }
    
    return info;
}

// ============================================================================
// ProjectSerializer Implementation
// ============================================================================

std::string ProjectSerializer::serialize(const std::string& projectData,
                                         const SerializeOptions& options) {
    // En producción, usar parser JSON real
    // Aquí retornamos los datos tal cual (simplificación)
    return projectData;
}

std::pair<std::string, SchemaValidationResult> ProjectSerializer::deserialize(const std::string& jsonContent) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    auto validationResult = schemaMgr.validate(jsonContent);
    
    return {jsonContent, validationResult};
}

bool ProjectSerializer::saveToFile(const std::string& filePath,
                                   const std::string& projectData,
                                   const SerializeOptions& options) {
    // Validar antes de guardar
    if (options.validateBeforeSerialize) {
        auto& schemaMgr = ProjectSchemaManager::instance();
        auto result = schemaMgr.validate(projectData);
        if (!result.isValid) {
            return false;
        }
    }
    
    // Crear backup
    createBackup(filePath);
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << projectData;
    return file.good();
}

std::pair<std::string, SchemaValidationResult> ProjectSerializer::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        SchemaValidationResult error;
        error.errors.push_back("No se pudo abrir el archivo: " + filePath);
        return {"", error};
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Migrar automáticamente si es necesario
    auto& schemaMgr = ProjectSchemaManager::instance();
    if (schemaMgr.migrateToCurrent(content)) {
        // Migración exitosa
    }
    
    auto validationResult = schemaMgr.validate(content);
    return {content, validationResult};
}

std::string ProjectSerializer::createBackup(const std::string& filePath) {
    // Generar nombre de backup con timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    std::string timestamp = ss.str();
    
    std::string backupPath = filePath + ".backup." + timestamp;
    
    // Copiar archivo original a backup
    std::ifstream src(filePath, std::ios::binary);
    if (!src.is_open()) {
        return "";
    }
    
    std::ofstream dst(backupPath, std::ios::binary);
    if (!dst.is_open()) {
        return "";
    }
    
    dst << src.rdbuf();
    return backupPath;
}

bool ProjectSerializer::restoreFromBackup(const std::string& backupPath, const std::string& targetPath) {
    std::ifstream src(backupPath, std::ios::binary);
    if (!src.is_open()) {
        return false;
    }
    
    std::ofstream dst(targetPath, std::ios::binary);
    if (!dst.is_open()) {
        return false;
    }
    
    dst << src.rdbuf();
    return dst.good();
}

// ============================================================================
// ProjectValidator Implementation
// ============================================================================

ProjectValidator::ValidationReport ProjectValidator::validate(const std::string& projectData) {
    ValidationReport report;
    
    // Validar IDs únicos
    if (!validateUniqueIds(projectData)) {
        report.errors.push_back("IDs duplicados encontrados");
        report.hasErrors = true;
    }
    
    // Validar referencias cruzadas
    if (!validateCrossReferences(projectData)) {
        report.errors.push_back("Referencias cruzadas inválidas");
        report.hasErrors = true;
    }
    
    // Validar timings
    if (!validateTimings(projectData)) {
        report.errors.push_back("Tiempos inválidos");
        report.hasErrors = true;
    }
    
    // Validar assets
    if (!validateAssets(projectData)) {
        report.warnings.push_back("Algunos assets no están disponibles");
        report.hasWarnings = true;
    }
    
    report.isValid = !report.hasErrors;
    return report;
}

bool ProjectValidator::validateUniqueIds(const std::string& projectData) {
    // Extraer todos los IDs y verificar unicidad
    std::regex idRegex(R"("id"\s*:\s*"([^"]+)")");
    std::set<std::string> ids;
    
    auto begin = std::sregex_iterator(projectData.begin(), projectData.end(), idRegex);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        std::string id = (*it)[1].str();
        if (ids.find(id) != ids.end()) {
            return false; // ID duplicado
        }
        ids.insert(id);
    }
    
    return true;
}

bool ProjectValidator::validateCrossReferences(const std::string& projectData) {
    // Verificar que todas las referencias a assets/clips existan
    // Implementación simplificada
    return true;
}

bool ProjectValidator::validateTimings(const std::string& projectData) {
    // Verificar que los tiempos sean consistentes
    // Implementación simplificada
    return true;
}

bool ProjectValidator::validateAssets(const std::string& projectData) {
    // Verificar existencia de archivos referenciados
    // Implementación simplificada
    return true;
}

std::pair<std::string, ProjectValidator::ValidationReport> ProjectValidator::autoFix(const std::string& projectData) {
    ValidationReport report;
    std::string fixed = projectData;
    
    // Auto-fixes comunes podrían aplicarse aquí
    
    report.fixedIssues = report.errors; // Simular que se arreglaron los errores
    report.errors.clear();
    report.isValid = true;
    
    return {fixed, report};
}

// ============================================================================
// ProjectFixtures Implementation
// ============================================================================

std::string ProjectFixtures::createMinimalValidProject() {
    return R"({
    "schemaVersion": "1.4.0",
    "id": "project-minimal-001",
    "name": "Minimal Project",
    "description": "",
    "createdAt": "2024-01-01T00:00:00Z",
    "updatedAt": "2024-01-01T00:00:00Z",
    "videoTracks": [],
    "audioTracks": [],
    "clips": [],
    "assets": [],
    "effects": [],
    "transitions": [],
    "subtitles": [],
    "markers": [],
    "settings": {},
    "metadata": {}
})";
}

std::string ProjectFixtures::createFullValidProject() {
    return R"({
    "schemaVersion": "1.4.0",
    "id": "project-full-001",
    "name": "Full Featured Project",
    "description": "A complete project with all features",
    "createdAt": "2024-01-01T00:00:00Z",
    "updatedAt": "2024-01-01T00:00:00Z",
    "videoTracks": [
        {
            "id": "track-video-001",
            "name": "Video Track 1",
            "enabled": true,
            "locked": false,
            "opacity": 1.0
        }
    ],
    "audioTracks": [
        {
            "id": "track-audio-001",
            "name": "Audio Track 1",
            "enabled": true,
            "locked": false,
            "volume": 1.0
        }
    ],
    "clips": [],
    "assets": [],
    "effects": [],
    "transitions": [],
    "subtitles": [],
    "markers": [],
    "settings": {
        "width": 1920,
        "height": 1080,
        "fps": 30,
        "duration": 60000
    },
    "metadata": {
        "author": "CCOS Test Suite",
        "version": "1.0"
    }
})";
}

std::string ProjectFixtures::createLegacyProject(ProjectSchemaVersion version) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    std::string versionStr = schemaMgr.versionToString(version);
    
    return R"({
    "schemaVersion": ")" + versionStr + R"(",
    "id": "project-legacy-001",
    "name": "Legacy Project",
    "createdAt": "2024-01-01T00:00:00Z",
    "videoTracks": [],
    "clips": [],
    "assets": []
})";
}

std::string ProjectFixtures::createCorruptedProject(const std::string& corruptionType) {
    if (corruptionType == "missing_brace") {
        return R"({
    "schemaVersion": "1.4.0",
    "id": "corrupted-001"
    // Missing closing brace
)";
    }
    if (corruptionType == "missing_bracket") {
        return R"({
    "schemaVersion": "1.4.0",
    "id": "corrupted-002",
    "clips": [
        {"id": "clip-1"}
    // Missing closing bracket
})";
    }
    if (corruptionType == "empty") {
        return "";
    }
    if (corruptionType == "invalid_json") {
        return "{ invalid json content }";
    }
    
    return createMinimalValidProject();
}

std::string ProjectFixtures::createProjectWithUnknownFields() {
    return R"({
    "schemaVersion": "1.4.0",
    "id": "project-unknown-fields-001",
    "name": "Project with Unknown Fields",
    "createdAt": "2024-01-01T00:00:00Z",
    "unknownField1": "value1",
    "unknownField2": 123,
    "unknownField3": {"nested": "object"},
    "videoTracks": [],
    "clips": [],
    "assets": []
})";
}

std::string ProjectFixtures::createLargeProject(int clipCount) {
    std::ostringstream oss;
    oss << R"({
    "schemaVersion": "1.4.0",
    "id": "project-large-001",
    "name": "Large Project",
    "createdAt": "2024-01-01T00:00:00Z",
    "videoTracks": [{"id": "track-1", "name": "Track 1"}],
    "audioTracks": [],
    "clips": [";
    
    for (int i = 0; i < clipCount; ++i) {
        if (i > 0) oss << ",";
        oss << R"({"id": "clip-)" << i << R"(", "name": "Clip )" << i << R"("})";
    }
    
    oss << R"(],
    "assets": [],
    "effects": [],
    "transitions": [],
    "subtitles": [],
    "markers": [],
    "settings": {},
    "metadata": {}
})";
    
    return oss.str();
}

std::string ProjectFixtures::createEmptyProject() {
    return R"({})";
}

} // namespace ccos
