#include <gtest/gtest.h>
#include "project/ProjectSchema.hpp"

using namespace ccos;

// ============================================================================
// Tests para ProjectSchemaManager
// ============================================================================

TEST(ProjectSchemaTest, SchemaVersionParsing) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    // Test parsing de versiones válidas
    EXPECT_EQ(schemaMgr.parseVersion("1.0.0"), ProjectSchemaVersion::V1_0_0);
    EXPECT_EQ(schemaMgr.parseVersion("1.0"), ProjectSchemaVersion::V1_0_0);
    EXPECT_EQ(schemaMgr.parseVersion("1.1.0"), ProjectSchemaVersion::V1_1_0);
    EXPECT_EQ(schemaMgr.parseVersion("1.2.0"), ProjectSchemaVersion::V1_2_0);
    EXPECT_EQ(schemaMgr.parseVersion("1.3.0"), ProjectSchemaVersion::V1_3_0);
    EXPECT_EQ(schemaMgr.parseVersion("1.4.0"), ProjectSchemaVersion::V1_4_0);
    
    // Test versión inválida
    EXPECT_FALSE(schemaMgr.parseVersion("invalid").has_value());
    EXPECT_FALSE(schemaMgr.parseVersion("").has_value());
}

TEST(ProjectSchemaTest, VersionToString) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    EXPECT_EQ(schemaMgr.versionToString(ProjectSchemaVersion::V1_0_0), "1.0.0");
    EXPECT_EQ(schemaMgr.versionToString(ProjectSchemaVersion::V1_1_0), "1.1.0");
    EXPECT_EQ(schemaMgr.versionToString(ProjectSchemaVersion::V1_2_0), "1.2.0");
    EXPECT_EQ(schemaMgr.versionToString(ProjectSchemaVersion::V1_3_0), "1.3.0");
    EXPECT_EQ(schemaMgr.versionToString(ProjectSchemaVersion::V1_4_0), "1.4.0");
    EXPECT_EQ(schemaMgr.versionToString(ProjectSchemaVersion::UNKNOWN), "unknown");
}

TEST(ProjectSchemaTest, DetectVersion) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    // Test detección por campo schemaVersion
    std::string json1 = R"({"schemaVersion": "1.0.0", "id": "test"})";
    auto version1 = schemaMgr.detectVersion(json1);
    EXPECT_TRUE(version1.has_value());
    EXPECT_EQ(version1.value(), ProjectSchemaVersion::V1_0_0);
    
    // Test detección por características (sin schemaVersion)
    std::string json2 = R"({"keyframes": [], "clips": []})";
    auto version2 = schemaMgr.detectVersion(json2);
    EXPECT_TRUE(version2.has_value());
    EXPECT_EQ(version2.value(), ProjectSchemaVersion::V1_4_0);
    
    std::string json3 = R"({"subtitles": [], "clips": []})";
    auto version3 = schemaMgr.detectVersion(json3);
    EXPECT_TRUE(version3.has_value());
    EXPECT_EQ(version3.value(), ProjectSchemaVersion::V1_3_0);
    
    std::string json4 = R"({"effects": [], "clips": []})";
    auto version4 = schemaMgr.detectVersion(json4);
    EXPECT_TRUE(version4.has_value());
    EXPECT_EQ(version4.value(), ProjectSchemaVersion::V1_2_0);
    
    std::string json5 = R"({"audioTracks": [], "clips": []})";
    auto version5 = schemaMgr.detectVersion(json5);
    EXPECT_TRUE(version5.has_value());
    EXPECT_EQ(version5.value(), ProjectSchemaVersion::V1_1_0);
}

TEST(ProjectSchemaTest, ValidateValidProject) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    std::string validProject = ProjectFixtures::createMinimalValidProject();
    
    auto result = schemaMgr.validate(validProject);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.version, "1.4.0");
    EXPECT_TRUE(result.errors.empty());
}

TEST(ProjectSchemaTest, ValidateEmptyProject) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    auto result = schemaMgr.validate("");
    
    EXPECT_FALSE(result.isValid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "Contenido vacío");
}

TEST(ProjectSchemaTest, ValidateCorruptedProject) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    std::string corruptedProject = ProjectFixtures::createCorruptedProject("missing_brace");
    
    auto result = schemaMgr.validate(corruptedProject);
    
    EXPECT_FALSE(result.isValid);
    EXPECT_FALSE(result.errors.empty());
}

TEST(ProjectSchemaTest, ValidateUnknownFields) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    std::string projectWithUnknown = ProjectFixtures::createProjectWithUnknownFields();
    
    auto result = schemaMgr.validate(projectWithUnknown);
    
    EXPECT_TRUE(result.isValid); // Es válido pero con warnings
    EXPECT_FALSE(result.warnings.empty());
    EXPECT_FALSE(result.unknownFields.empty());
}

TEST(ProjectSchemaTest, IsCorrupted) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    EXPECT_FALSE(schemaMgr.isCorrupted(ProjectFixtures::createMinimalValidProject()));
    EXPECT_TRUE(schemaMgr.isCorrupted(""));
    EXPECT_TRUE(schemaMgr.isCorrupted("{"));
    EXPECT_TRUE(schemaMgr.isCorrupted("{}["));
}

TEST(ProjectSchemaTest, AttemptRepair) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    // Proyecto no corrupto no necesita reparación
    std::string validProject = ProjectFixtures::createMinimalValidProject();
    auto repaired1 = schemaMgr.attemptRepair(validProject);
    EXPECT_TRUE(repaired1.has_value());
    EXPECT_EQ(repaired1.value(), validProject);
    
    // Proyecto corrupto puede ser reparado
    std::string corrupted = "{\"schemaVersion\": \"1.0.0\"";
    auto repaired2 = schemaMgr.attemptRepair(corrupted);
    EXPECT_TRUE(repaired2.has_value());
    EXPECT_FALSE(schemaMgr.isCorrupted(repaired2.value()));
}

TEST(ProjectSchemaTest, MigrationV1ToCurrent) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    std::string legacyProject = ProjectFixtures::createLegacyProject(ProjectSchemaVersion::V1_0_0);
    
    // Verificar que es V1.0.0
    auto detected = schemaMgr.detectVersion(legacyProject);
    EXPECT_EQ(detected.value(), ProjectSchemaVersion::V1_0_0);
    
    // Migrar a current
    bool migrated = schemaMgr.migrateToCurrent(legacyProject);
    EXPECT_TRUE(migrated);
    
    // Verificar que ahora es la versión actual
    auto newDetected = schemaMgr.detectVersion(legacyProject);
    EXPECT_EQ(newDetected.value(), ProjectSchemaVersion::CURRENT);
}

TEST(ProjectSchemaTest, CompatibilityCheck) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    // Proyecto actual debe ser compatible
    std::string currentProject = ProjectFixtures::createMinimalValidProject();
    auto compat1 = schemaMgr.checkCompatibility(currentProject);
    EXPECT_TRUE(compat1.isCompatible);
    EXPECT_FALSE(compat1.requiresMigration);
    
    // Proyecto legacy debe requerir migración
    std::string legacyProject = ProjectFixtures::createLegacyProject(ProjectSchemaVersion::V1_0_0);
    auto compat2 = schemaMgr.checkCompatibility(legacyProject);
    EXPECT_TRUE(compat2.isCompatible);
    EXPECT_TRUE(compat2.requiresMigration);
}

// ============================================================================
// Tests para ProjectSerializer
// ============================================================================

TEST(ProjectSerializerTest, SerializeDeserialize) {
    std::string original = ProjectFixtures::createMinimalValidProject();
    
    auto serialized = ProjectSerializer::serialize(original);
    auto [deserialized, result] = ProjectSerializer::deserialize(serialized);
    
    EXPECT_TRUE(result.isValid);
}

TEST(ProjectSerializerTest, SaveAndLoadFile) {
    std::string testPath = "/tmp/test_project.ccos";
    std::string project = ProjectFixtures::createMinimalValidProject();
    
    // Guardar
    bool saved = ProjectSerializer::saveToFile(testPath, project);
    EXPECT_TRUE(saved);
    
    // Cargar
    auto [loaded, result] = ProjectSerializer::loadFromFile(testPath);
    EXPECT_TRUE(result.isValid);
    EXPECT_FALSE(loaded.empty());
}

TEST(ProjectSerializerTest, CreateBackup) {
    std::string testPath = "/tmp/test_project_backup.ccos";
    std::string project = ProjectFixtures::createMinimalValidProject();
    
    // Primero guardar el archivo original
    ProjectSerializer::saveToFile(testPath, project);
    
    // Crear backup
    std::string backupPath = ProjectSerializer::createBackup(testPath);
    EXPECT_FALSE(backupPath.empty());
    EXPECT_NE(backupPath, testPath);
}

// ============================================================================
// Tests para ProjectValidator
// ============================================================================

TEST(ProjectValidatorTest, ValidateUniqueIdsValid) {
    std::string validProject = ProjectFixtures::createMinimalValidProject();
    EXPECT_TRUE(ProjectValidator::validateUniqueIds(validProject));
}

TEST(ProjectValidatorTest, ValidateFullProject) {
    std::string fullProject = ProjectFixtures::createFullValidProject();
    auto report = ProjectValidator::validate(fullProject);
    
    EXPECT_TRUE(report.isValid);
    EXPECT_FALSE(report.hasErrors);
}

TEST(ProjectValidatorTest, AutoFix) {
    std::string project = ProjectFixtures::createMinimalValidProject();
    auto [fixed, report] = ProjectValidator::autoFix(project);
    
    EXPECT_TRUE(report.isValid);
}

// ============================================================================
// Tests para ProjectFixtures
// ============================================================================

TEST(ProjectFixturesTest, CreateMinimalValidProject) {
    std::string project = ProjectFixtures::createMinimalValidProject();
    auto& schemaMgr = ProjectSchemaManager::instance();
    auto result = schemaMgr.validate(project);
    
    EXPECT_TRUE(result.isValid);
}

TEST(ProjectFixturesTest, CreateFullValidProject) {
    std::string project = ProjectFixtures::createFullValidProject();
    auto& schemaMgr = ProjectSchemaManager::instance();
    auto result = schemaMgr.validate(project);
    
    EXPECT_TRUE(result.isValid);
}

TEST(ProjectFixturesTest, CreateLegacyProject) {
    std::string legacy = ProjectFixtures::createLegacyProject(ProjectSchemaVersion::V1_0_0);
    auto& schemaMgr = ProjectSchemaManager::instance();
    auto detected = schemaMgr.detectVersion(legacy);
    
    EXPECT_EQ(detected.value(), ProjectSchemaVersion::V1_0_0);
}

TEST(ProjectFixturesTest, CreateCorruptedProjects) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    std::string corrupted1 = ProjectFixtures::createCorruptedProject("missing_brace");
    EXPECT_TRUE(schemaMgr.isCorrupted(corrupted1));
    
    std::string corrupted2 = ProjectFixtures::createCorruptedProject("missing_bracket");
    EXPECT_TRUE(schemaMgr.isCorrupted(corrupted2));
    
    std::string corrupted3 = ProjectFixtures::createCorruptedProject("empty");
    EXPECT_TRUE(schemaMgr.isCorrupted(corrupted3));
}

TEST(ProjectFixturesTest, CreateLargeProject) {
    std::string largeProject = ProjectFixtures::createLargeProject(100);
    auto& schemaMgr = ProjectSchemaManager::instance();
    auto result = schemaMgr.validate(largeProject);
    
    // Debería ser válido a pesar de ser grande
    EXPECT_TRUE(result.isValid);
}

TEST(ProjectFixturesTest, CreateEmptyProject) {
    std::string emptyProject = ProjectFixtures::createEmptyProject();
    auto& schemaMgr = ProjectSchemaManager::instance();
    auto result = schemaMgr.validate(emptyProject);
    
    // Proyecto vacío debería ser inválido (faltan campos requeridos)
    EXPECT_FALSE(result.isValid);
}

// ============================================================================
// Tests de Roundtrip
// ============================================================================

TEST(ProjectSchemaRoundtripTest, SaveLoadRoundtrip) {
    std::string original = ProjectFixtures::createFullValidProject();
    std::string testPath = "/tmp/test_roundtrip.ccos";
    
    // Guardar
    EXPECT_TRUE(ProjectSerializer::saveToFile(testPath, original));
    
    // Cargar
    auto [loaded, result] = ProjectSerializer::loadFromFile(testPath);
    EXPECT_TRUE(result.isValid);
    
    // El contenido cargado debería ser válido
    auto& schemaMgr = ProjectSchemaManager::instance();
    auto validationResult = schemaMgr.validate(loaded);
    EXPECT_TRUE(validationResult.isValid);
}

TEST(ProjectSchemaRoundtripTest, MigrationRoundtrip) {
    auto& schemaMgr = ProjectSchemaManager::instance();
    
    // Crear proyecto V1.0.0
    std::string legacy = ProjectFixtures::createLegacyProject(ProjectSchemaVersion::V1_0_0);
    EXPECT_EQ(schemaMgr.detectVersion(legacy).value(), ProjectSchemaVersion::V1_0_0);
    
    // Migrar a current
    EXPECT_TRUE(schemaMgr.migrateToCurrent(legacy));
    EXPECT_EQ(schemaMgr.detectVersion(legacy).value(), ProjectSchemaVersion::CURRENT);
    
    // Validar que sigue siendo válido después de migrar
    auto result = schemaMgr.validate(legacy);
    EXPECT_TRUE(result.isValid);
}
