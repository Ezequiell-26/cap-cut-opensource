#include "ProjectSchema.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ccos {
namespace {

using Json = nlohmann::json;

std::optional<Json> parseJson(const std::string& content) {
    if (content.empty()) return std::nullopt;
    try {
        return Json::parse(content);
    } catch (const Json::exception&) {
        return std::nullopt;
    }
}

std::string nowBackupStamp() {
    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return std::to_string(millis);
}

void collectIds(const Json& value, std::set<std::string>& ids, bool& unique) {
    if (value.is_object()) {
        for (const auto& [key, child] : value.items()) {
            if (key == "id" && child.is_string()) {
                const auto id = child.get<std::string>();
                if (!ids.insert(id).second) unique = false;
            }
            collectIds(child, ids, unique);
        }
    } else if (value.is_array()) {
        for (const auto& child : value) collectIds(child, ids, unique);
    }
}

void collectUnknownRootFields(const Json& root,
                              std::vector<std::string>& unknownFields,
                              std::vector<std::string>& warnings) {
    static const std::set<std::string> knownFields = {
        "schemaVersion", "id", "name", "description", "createdAt", "updatedAt",
        "videoTracks", "audioTracks", "clips", "assets", "effects", "transitions",
        "subtitles", "markers", "settings", "metadata", "keyframes"
    };

    for (const auto& [key, value] : root.items()) {
        (void)value;
        if (knownFields.find(key) == knownFields.end()) {
            unknownFields.push_back(key);
            warnings.push_back("Campo desconocido: " + key);
        }
    }
}

bool isBalancedJsonPrefix(const std::string& text) {
    int braces = 0;
    int brackets = 0;
    bool inString = false;
    bool escaped = false;

    for (const char ch : text) {
        if (escaped) {
            escaped = false;
            continue;
        }
        if (inString && ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (inString) continue;

        if (ch == '{') ++braces;
        else if (ch == '}') {
            if (--braces < 0) return false;
        } else if (ch == '[') ++brackets;
        else if (ch == ']') {
            if (--brackets < 0) return false;
        }
    }

    return !inString && braces >= 0 && brackets >= 0;
}

void appendMissingClosers(std::string& text) {
    int braces = 0;
    int brackets = 0;
    bool inString = false;
    bool escaped = false;

    for (const char ch : text) {
        if (escaped) {
            escaped = false;
            continue;
        }
        if (inString && ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (inString) continue;
        if (ch == '{') ++braces;
        else if (ch == '}') --braces;
        else if (ch == '[') ++brackets;
        else if (ch == ']') --brackets;
    }

    if (inString || braces < 0 || brackets < 0) return;

    while (brackets-- > 0) text.push_back(']');
    while (braces-- > 0) text.push_back('}');
}

bool addEmptyArrayField(std::string& content, const char* field, const char* version) {
    auto parsed = parseJson(content);
    if (!parsed || !parsed->is_object()) return false;
    Json root = std::move(*parsed);
    if (!root.contains(field)) root[field] = Json::array();
    root["schemaVersion"] = version;
    content = root.dump(2) + "\n";
    return true;
}

} // namespace

std::string SchemaValidationResult::toString() const {
    std::ostringstream oss;
    oss << "Schema Validation Result:\n";
    oss << "  Valid: " << (isValid ? "YES" : "NO") << "\n";
    oss << "  Version: " << version << "\n";

    if (!errors.empty()) {
        oss << "  Errors (" << errors.size() << "):\n";
        for (const auto& err : errors) oss << "    - " << err << "\n";
    }
    if (!warnings.empty()) {
        oss << "  Warnings (" << warnings.size() << "):\n";
        for (const auto& warning : warnings) oss << "    - " << warning << "\n";
    }
    if (!unknownFields.empty()) {
        oss << "  Unknown Fields (" << unknownFields.size() << "):\n";
        for (const auto& field : unknownFields) oss << "    - " << field << "\n";
    }
    return oss.str();
}

std::string ProjectValidator::ValidationReport::summary() const {
    std::ostringstream oss;
    oss << "Validation Summary: " << (isValid ? "PASSED" : "FAILED")
        << " | Errors: " << errors.size()
        << " | Warnings: " << warnings.size();
    if (!fixedIssues.empty()) oss << " | Fixed: " << fixedIssues.size();
    return oss.str();
}

ProjectSchemaManager& ProjectSchemaManager::instance() {
    static ProjectSchemaManager instance;
    return instance;
}

ProjectSchemaManager::ProjectSchemaManager() {
    initializeDefaultMigrations();
}

ProjectSchemaManager::~ProjectSchemaManager() = default;

void ProjectSchemaManager::initializeDefaultMigrations() {
    MigrationInfo v1_0_0_to_v1_1_0;
    v1_0_0_to_v1_1_0.fromVersion = ProjectSchemaVersion::V1_0_0;
    v1_0_0_to_v1_1_0.toVersion = ProjectSchemaVersion::V1_1_0;
    v1_0_0_to_v1_1_0.description = "Agregar soporte para múltiples pistas de audio";
    v1_0_0_to_v1_1_0.isBreaking = false;
    v1_0_0_to_v1_1_0.migrateFunction = [](const std::string& input, std::string& output) {
        output = input;
        return addEmptyArrayField(output, "audioTracks", "1.1.0");
    };
    migrations_.push_back(v1_0_0_to_v1_1_0);

    MigrationInfo v1_1_0_to_v1_2_0;
    v1_1_0_to_v1_2_0.fromVersion = ProjectSchemaVersion::V1_1_0;
    v1_1_0_to_v1_2_0.toVersion = ProjectSchemaVersion::V1_2_0;
    v1_1_0_to_v1_2_0.description = "Agregar soporte para efectos y transiciones";
    v1_1_0_to_v1_2_0.isBreaking = false;
    v1_1_0_to_v1_2_0.migrateFunction = [](const std::string& input, std::string& output) {
        output = input;
        auto parsed = parseJson(output);
        if (!parsed || !parsed->is_object()) return false;
        Json root = std::move(*parsed);
        if (!root.contains("effects")) root["effects"] = Json::array();
        if (!root.contains("transitions")) root["transitions"] = Json::array();
        root["schemaVersion"] = "1.2.0";
        output = root.dump(2) + "\n";
        return true;
    };
    migrations_.push_back(v1_1_0_to_v1_2_0);

    MigrationInfo v1_2_0_to_v1_3_0;
    v1_2_0_to_v1_3_0.fromVersion = ProjectSchemaVersion::V1_2_0;
    v1_2_0_to_v1_3_0.toVersion = ProjectSchemaVersion::V1_3_0;
    v1_2_0_to_v1_3_0.description = "Agregar soporte para subtítulos";
    v1_2_0_to_v1_3_0.isBreaking = false;
    v1_2_0_to_v1_3_0.migrateFunction = [](const std::string& input, std::string& output) {
        output = input;
        return addEmptyArrayField(output, "subtitles", "1.3.0");
    };
    migrations_.push_back(v1_2_0_to_v1_3_0);

    MigrationInfo v1_3_0_to_v1_4_0;
    v1_3_0_to_v1_4_0.fromVersion = ProjectSchemaVersion::V1_3_0;
    v1_3_0_to_v1_4_0.toVersion = ProjectSchemaVersion::V1_4_0;
    v1_3_0_to_v1_4_0.description = "Agregar soporte para keyframes en efectos";
    v1_3_0_to_v1_4_0.isBreaking = false;
    v1_3_0_to_v1_4_0.migrateFunction = [](const std::string& input, std::string& output) {
        output = input;
        return addEmptyArrayField(output, "keyframes", "1.4.0");
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
    const auto parsed = parseJson(jsonContent);
    if (!parsed || !parsed->is_object()) return std::nullopt;

    const Json& root = *parsed;
    if (root.contains("schemaVersion") && root["schemaVersion"].is_string()) {
        return parseVersion(root["schemaVersion"].get<std::string>());
    }

    if (root.contains("keyframes")) return ProjectSchemaVersion::V1_4_0;
    if (root.contains("subtitles")) return ProjectSchemaVersion::V1_3_0;
    if (root.contains("effects") || root.contains("transitions")) return ProjectSchemaVersion::V1_2_0;
    if (root.contains("audioTracks")) return ProjectSchemaVersion::V1_1_0;
    return ProjectSchemaVersion::V1_0_0;
}

SchemaValidationResult ProjectSchemaManager::validate(const std::string& jsonContent) const {
    SchemaValidationResult result;
    if (jsonContent.empty()) {
        result.errors.push_back("Contenido vacío");
        return result;
    }

    const auto parsed = parseJson(jsonContent);
    if (!parsed) {
        result.errors.push_back("JSON inválido");
        return result;
    }
    if (!parsed->is_object()) {
        result.errors.push_back("La raíz del proyecto debe ser un objeto JSON");
        return result;
    }

    const auto detectedVersion = detectVersion(jsonContent);
    if (!detectedVersion.has_value()) {
        result.errors.push_back("No se pudo detectar la versión del schema");
        return result;
    }
    result.version = versionToString(*detectedVersion);

    if (!parsed->contains("schemaVersion")) {
        result.warnings.push_back("Falta schemaVersion; se utilizará detección por compatibilidad");
    } else if (!(*parsed)["schemaVersion"].is_string()) {
        result.errors.push_back("schemaVersion debe ser una cadena");
    }

    for (const auto* field : {"id", "name", "createdAt"}) {
        if (!parsed->contains(field)) {
            result.errors.push_back(std::string("Campo requerido faltante: ") + field);
        } else if (!(*parsed)[field].is_string()) {
            result.errors.push_back(std::string("Campo requerido inválido: ") + field);
        }
    }

    collectUnknownRootFields(*parsed, result.unknownFields, result.warnings);
    result.isValid = result.errors.empty();
    return result;
}

bool ProjectSchemaManager::migrateToCurrent(std::string& jsonContent) {
    return migrateTo(jsonContent, ProjectSchemaVersion::CURRENT);
}

bool ProjectSchemaManager::migrateTo(std::string& jsonContent, ProjectSchemaVersion targetVersion) {
    auto currentDetected = detectVersion(jsonContent);
    if (!currentDetected.has_value()) return false;
    if (currentDetected == targetVersion) return true;
    if (*currentDetected > targetVersion) return false;

    std::string workingContent = jsonContent;
    auto workingVersion = *currentDetected;

    while (workingVersion < targetVersion) {
        bool migrationFound = false;
        for (const auto& migration : migrations_) {
            if (migration.fromVersion != workingVersion || migration.toVersion > targetVersion) continue;
            std::string nextContent;
            if (!migration.migrateFunction || !migration.migrateFunction(workingContent, nextContent)) return false;
            if (!detectVersion(nextContent).has_value()) return false;
            workingContent = std::move(nextContent);
            workingVersion = migration.toVersion;
            migrationFound = true;
            break;
        }
        if (!migrationFound) return false;
    }

    jsonContent = std::move(workingContent);
    return true;
}

void ProjectSchemaManager::registerMigration(MigrationInfo info) {
    if (!info.migrateFunction) return;
    migrations_.push_back(std::move(info));
    std::sort(migrations_.begin(), migrations_.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.fromVersion != rhs.fromVersion) return lhs.fromVersion < rhs.fromVersion;
        return lhs.toVersion < rhs.toVersion;
    });
}

bool ProjectSchemaManager::isCorrupted(const std::string& jsonContent) const {
    if (jsonContent.empty()) return true;
    if (!isBalancedJsonPrefix(jsonContent)) return true;
    return !parseJson(jsonContent).has_value();
}

std::optional<std::string> ProjectSchemaManager::attemptRepair(const std::string& jsonContent) const {
    if (jsonContent.empty()) return std::nullopt;
    if (!isCorrupted(jsonContent)) return jsonContent;
    if (!isBalancedJsonPrefix(jsonContent)) return std::nullopt;

    std::string repaired = jsonContent;
    appendMissingClosers(repaired);
    const auto parsed = parseJson(repaired);
    if (!parsed) return std::nullopt;
    return parsed->dump(2) + "\n";
}

ProjectSchemaManager::CompatibilityInfo ProjectSchemaManager::checkCompatibility(const std::string& jsonContent) const {
    CompatibilityInfo info;
    info.currentVersion = currentVersion_;

    const auto detectedVersion = detectVersion(jsonContent);
    if (!detectedVersion.has_value()) {
        info.message = "No se pudo detectar la versión del proyecto";
        return info;
    }

    info.projectVersion = *detectedVersion;
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

std::string ProjectSerializer::serialize(const std::string& projectData, const SerializeOptions& options) {
    auto parsed = parseJson(projectData);
    if (!parsed || !parsed->is_object()) return {};

    std::string normalized = parsed->dump(options.prettyPrint ? 2 : -1);
    auto& schemaMgr = ProjectSchemaManager::instance();
    if (options.targetVersion != ProjectSchemaVersion::UNKNOWN) {
        if (!schemaMgr.migrateTo(normalized, options.targetVersion)) {
            if (!schemaMgr.detectVersion(normalized).has_value()) return {};
        }
    }

    if (!options.includeMetadata) {
        const auto normalizedParsed = parseJson(normalized);
        if (!normalizedParsed || !normalizedParsed->is_object()) return {};
        Json root = std::move(*normalizedParsed);
        root.erase("metadata");
        normalized = root.dump(options.prettyPrint ? 2 : -1);
    }

    if (options.validateBeforeSerialize && !schemaMgr.validate(normalized).isValid) return {};
    if (!normalized.empty() && normalized.back() != '\n') normalized.push_back('\n');
    return normalized;
}

std::pair<std::string, SchemaValidationResult> ProjectSerializer::deserialize(const std::string& jsonContent) {
    std::string content = jsonContent;
    auto& schemaMgr = ProjectSchemaManager::instance();
    if (!schemaMgr.migrateToCurrent(content)) {
        SchemaValidationResult error;
        error.errors.push_back("No se pudo migrar el proyecto al schema actual");
        return {jsonContent, error};
    }
    const auto validationResult = schemaMgr.validate(content);
    return {content, validationResult};
}

bool ProjectSerializer::saveToFile(const std::string& filePath,
                                   const std::string& projectData,
                                   const SerializeOptions& options) {
    const std::string serialized = serialize(projectData, options);
    if (serialized.empty()) return false;

    createBackup(filePath);

    const std::string tempPath = filePath + ".tmp." + nowBackupStamp();
    {
        std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        file.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
        file.flush();
        if (!file.good()) {
            file.close();
            std::error_code removeError;
            std::filesystem::remove(tempPath, removeError);
            return false;
        }
    }

    std::error_code renameError;
    std::filesystem::remove(filePath, renameError);
    renameError.clear();
    std::filesystem::rename(tempPath, filePath, renameError);
    if (renameError) {
        std::error_code removeError;
        std::filesystem::remove(tempPath, removeError);
        return false;
    }
    return true;
}

std::pair<std::string, SchemaValidationResult> ProjectSerializer::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        SchemaValidationResult error;
        error.errors.push_back("No se pudo abrir el archivo: " + filePath);
        return {"", error};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    if (!file.good() && !file.eof()) {
        SchemaValidationResult error;
        error.errors.push_back("Error leyendo el archivo: " + filePath);
        return {"", error};
    }

    return deserialize(buffer.str());
}

std::string ProjectSerializer::createBackup(const std::string& filePath) {
    std::ifstream src(filePath, std::ios::binary);
    if (!src.is_open()) return {};

    const std::string backupPath = filePath + ".backup." + nowBackupStamp();
    std::ofstream dst(backupPath, std::ios::binary | std::ios::trunc);
    if (!dst.is_open()) return {};

    dst << src.rdbuf();
    if (!dst.good()) {
        dst.close();
        std::error_code removeError;
        std::filesystem::remove(backupPath, removeError);
        return {};
    }
    return backupPath;
}

bool ProjectSerializer::restoreFromBackup(const std::string& backupPath, const std::string& targetPath) {
    std::ifstream src(backupPath, std::ios::binary);
    if (!src.is_open()) return false;

    const std::string tempPath = targetPath + ".restore." + nowBackupStamp();
    std::ofstream dst(tempPath, std::ios::binary | std::ios::trunc);
    if (!dst.is_open()) return false;

    dst << src.rdbuf();
    dst.flush();
    if (!dst.good()) {
        dst.close();
        std::error_code removeError;
        std::filesystem::remove(tempPath, removeError);
        return false;
    }
    dst.close();

    std::error_code renameError;
    std::filesystem::remove(targetPath, renameError);
    renameError.clear();
    std::filesystem::rename(tempPath, targetPath, renameError);
    if (renameError) {
        std::error_code removeError;
        std::filesystem::remove(tempPath, removeError);
        return false;
    }
    return true;
}

ProjectValidator::ValidationReport ProjectValidator::validate(const std::string& projectData) {
    ValidationReport report;
    const auto schema = ProjectSchemaManager::instance().validate(projectData);
    if (!schema.isValid) {
        report.errors = schema.errors;
        report.hasErrors = true;
        report.isValid = false;
        report.warnings = schema.warnings;
        report.hasWarnings = !report.warnings.empty();
        return report;
    }

    const auto parsed = parseJson(projectData);
    if (!parsed) {
        report.errors.push_back("JSON inválido");
        report.hasErrors = true;
        return report;
    }

    if (!validateUniqueIds(projectData)) {
        report.errors.push_back("IDs duplicados encontrados");
        report.hasErrors = true;
    }
    if (!validateCrossReferences(projectData)) {
        report.errors.push_back("Referencias cruzadas inválidas");
        report.hasErrors = true;
    }
    if (!validateTimings(projectData)) {
        report.errors.push_back("Tiempos inválidos");
        report.hasErrors = true;
    }
    if (!validateAssets(projectData)) {
        report.warnings.push_back("Algunos assets no están disponibles");
        report.hasWarnings = true;
    }

    report.isValid = !report.hasErrors;
    return report;
}

bool ProjectValidator::validateUniqueIds(const std::string& projectData) {
    const auto parsed = parseJson(projectData);
    if (!parsed) return false;
    std::set<std::string> ids;
    bool unique = true;
    collectIds(*parsed, ids, unique);
    return unique;
}

bool ProjectValidator::validateCrossReferences(const std::string& projectData) {
    const auto parsed = parseJson(projectData);
    if (!parsed || !parsed->is_object()) return false;

    std::set<std::string> assetIds;
    std::set<std::string> clipIds;
    std::set<std::string> trackIds;

    const Json& root = *parsed;
    if (root.contains("assets") && root["assets"].is_array()) {
        for (const auto& asset : root["assets"]) {
            if (asset.is_object() && asset.contains("id") && asset["id"].is_string()) assetIds.insert(asset["id"].get<std::string>());
        }
    }
    if (root.contains("clips") && root["clips"].is_array()) {
        for (const auto& clip : root["clips"]) {
            if (clip.is_object() && clip.contains("id") && clip["id"].is_string()) clipIds.insert(clip["id"].get<std::string>());
        }
    }
    for (const char* trackGroup : {"videoTracks", "audioTracks"}) {
        if (!root.contains(trackGroup) || !root[trackGroup].is_array()) continue;
        for (const auto& track : root[trackGroup]) {
            if (track.is_object() && track.contains("id") && track["id"].is_string()) trackIds.insert(track["id"].get<std::string>());
        }
    }

    bool valid = true;
    std::function<void(const Json&)> visit = [&](const Json& value) {
        if (value.is_object()) {
            for (const auto& [key, child] : value.items()) {
                if (child.is_string()) {
                    const auto ref = child.get<std::string>();
                    if (key == "assetId" && !assetIds.empty() && assetIds.count(ref) == 0) valid = false;
                    if (key == "clipId" && !clipIds.empty() && clipIds.count(ref) == 0) valid = false;
                    if (key == "trackId" && !trackIds.empty() && trackIds.count(ref) == 0) valid = false;
                }
                visit(child);
            }
        } else if (value.is_array()) {
            for (const auto& child : value) visit(child);
        }
    };
    visit(root);
    return valid;
}

bool ProjectValidator::validateTimings(const std::string& projectData) {
    const auto parsed = parseJson(projectData);
    if (!parsed) return false;

    bool valid = true;
    std::function<void(const Json&)> visit = [&](const Json& value) {
        if (value.is_object()) {
            double start = 0.0;
            double end = 0.0;
            bool hasStart = false;
            bool hasEnd = false;
            for (const auto& [key, child] : value.items()) {
                if (key == "start" || key == "startTime" || key == "startTimeMs") {
                    if (child.is_number()) {
                        start = child.get<double>();
                        hasStart = true;
                    }
                }
                if (key == "end" || key == "endTime" || key == "endTimeMs") {
                    if (child.is_number()) {
                        end = child.get<double>();
                        hasEnd = true;
                    }
                }
                visit(child);
            }
            if (hasStart && start < 0.0) valid = false;
            if (hasEnd && end < 0.0) valid = false;
            if (hasStart && hasEnd && end < start) valid = false;
        } else if (value.is_array()) {
            for (const auto& child : value) visit(child);
        }
    };
    visit(*parsed);
    return valid;
}

bool ProjectValidator::validateAssets(const std::string& projectData) {
    const auto parsed = parseJson(projectData);
    if (!parsed || !parsed->is_object()) return false;
    if (!parsed->contains("assets") || !(*parsed)["assets"].is_array()) return true;

    // Asset paths are external resources and may legitimately be unavailable on another machine.
    // We therefore validate their shape here, leaving physical availability as a warning layer.
    for (const auto& asset : (*parsed)["assets"]) {
        if (!asset.is_object()) return false;
        if (asset.contains("path") && !asset["path"].is_string()) return false;
    }
    return true;
}

std::pair<std::string, ProjectValidator::ValidationReport> ProjectValidator::autoFix(const std::string& projectData) {
    ValidationReport report = validate(projectData);
    if (!report.isValid) return {projectData, report};

    // Only report fixes that are actually applied. No silent mutation of IDs or timings.
    report.fixedIssues.clear();
    return {projectData, report};
}

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
    "metadata": {},
    "keyframes": []
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
        {"id": "track-video-001", "name": "Video Track 1", "enabled": true, "locked": false, "opacity": 1.0}
    ],
    "audioTracks": [
        {"id": "track-audio-001", "name": "Audio Track 1", "enabled": true, "locked": false, "volume": 1.0}
    ],
    "clips": [],
    "assets": [],
    "effects": [],
    "transitions": [],
    "subtitles": [],
    "markers": [],
    "settings": {"width": 1920, "height": 1080, "fps": 30, "duration": 60000},
    "metadata": {"author": "CCOS Test Suite", "version": "1.0"},
    "keyframes": []
})";
}

std::string ProjectFixtures::createLegacyProject(ProjectSchemaVersion version) {
    const auto versionStr = ProjectSchemaManager::instance().versionToString(version);
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
)";
    }
    if (corruptionType == "missing_bracket") {
        return R"({
    "schemaVersion": "1.4.0",
    "id": "corrupted-002",
    "clips": [
        {"id": "clip-1"}
})";
    }
    if (corruptionType == "empty") return {};
    if (corruptionType == "invalid_json") return "{ invalid json content }";
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
    "assets": [],
    "keyframes": []
})";
}

std::string ProjectFixtures::createLargeProject(int clipCount) {
    std::ostringstream oss;
    oss << "{\n"
        << "    \"schemaVersion\": \"1.4.0\",\n"
        << "    \"id\": \"project-large-001\",\n"
        << "    \"name\": \"Large Project\",\n"
        << "    \"createdAt\": \"2024-01-01T00:00:00Z\",\n"
        << "    \"videoTracks\": [{\"id\": \"track-1\", \"name\": \"Track 1\"}],\n"
        << "    \"audioTracks\": [],\n"
        << "    \"clips\": [\n";

    for (int i = 0; i < std::max(0, clipCount); ++i) {
        if (i > 0) oss << ",\n";
        oss << "        {\"id\": \"clip-" << i << "\", \"name\": \"Clip " << i << "\"}";
    }

    oss << "\n    ],\n"
        << "    \"assets\": [],\n"
        << "    \"effects\": [],\n"
        << "    \"transitions\": [],\n"
        << "    \"subtitles\": [],\n"
        << "    \"markers\": [],\n"
        << "    \"settings\": {},\n"
        << "    \"metadata\": {},\n"
        << "    \"keyframes\": []\n"
        << "}";
    return oss.str();
}

std::string ProjectFixtures::createEmptyProject() {
    return R"({})";
}

} // namespace ccos
