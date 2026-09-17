#include "plugins/PluginManager.hpp"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

namespace {

QString writeManifest(const QString& directory, const QString& library, const QString& extra = {}) {
    const QString path = QDir(directory).filePath(QStringLiteral("test.ccosplugin.json"));
    QFile file(path);
    EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray json = QStringLiteral(
        "{\"id\":\"test.plugin\",\"name\":\"Test Plugin\",\"version\":\"1.0.0\","
        "\"apiVersion\":\"1\",\"library\":\"%1\"%2}").arg(library, extra).toUtf8();
    file.write(json);
    file.close();
    return path;
}

QString writeLibrary(const QString& directory, const QString& extension) {
    const QString path = QDir(directory).filePath(QStringLiteral("plugin") + extension);
    QFile file(path);
    EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("not-a-real-library");
    file.close();
    return path;
}

} // namespace

TEST(PluginManagerSecurityTest, RejectsPathTraversal) {
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    const QString pluginDir = QDir(temp.path()).filePath(QStringLiteral("plugins"));
    ASSERT_TRUE(QDir().mkpath(pluginDir));

    writeManifest(pluginDir, QStringLiteral("../escape.dll"));
    QStringList diagnostics;
    const auto plugins = ccos::plugins::PluginManager::scan({pluginDir}, &diagnostics);

    EXPECT_TRUE(plugins.isEmpty());
    EXPECT_FALSE(diagnostics.isEmpty());
}

TEST(PluginManagerSecurityTest, RejectsUnsupportedLibraryExtension) {
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    const QString pluginDir = temp.path();
    writeLibrary(pluginDir, QStringLiteral(".txt"));
    writeManifest(pluginDir, QStringLiteral("plugin.txt"));

    const auto plugins = ccos::plugins::PluginManager::scan({pluginDir});
    EXPECT_TRUE(plugins.isEmpty());
}

TEST(PluginManagerSecurityTest, RequiresExistingLibrary) {
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
#if defined(Q_OS_WIN)
    const QString extension = QStringLiteral(".dll");
#elif defined(Q_OS_MACOS)
    const QString extension = QStringLiteral(".dylib");
#else
    const QString extension = QStringLiteral(".so");
#endif
    writeManifest(temp.path(), QStringLiteral("missing") + extension);

    QStringList diagnostics;
    const auto plugins = ccos::plugins::PluginManager::scan({temp.path()}, &diagnostics);
    EXPECT_TRUE(plugins.isEmpty());
    EXPECT_FALSE(diagnostics.isEmpty());
}

TEST(PluginManagerSecurityTest, AcceptsExistingCompatibleLibraryWithoutHash) {
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
#if defined(Q_OS_WIN)
    const QString extension = QStringLiteral(".dll");
#elif defined(Q_OS_MACOS)
    const QString extension = QStringLiteral(".dylib");
#else
    const QString extension = QStringLiteral(".so");
#endif

    writeLibrary(temp.path(), extension);
    writeManifest(temp.path(), QStringLiteral("plugin") + extension);

    QStringList diagnostics;
    const auto plugins = ccos::plugins::PluginManager::scan({temp.path()}, &diagnostics);
    ASSERT_EQ(plugins.size(), 1);
    EXPECT_TRUE(plugins.front().libraryExists);
    EXPECT_TRUE(plugins.front().resolvedLibraryPath.endsWith(QStringLiteral("plugin") + extension));
    EXPECT_TRUE(ccos::plugins::PluginManager::isCompatible(plugins.front()));
}

TEST(PluginManagerSecurityTest, HashMismatchIsRejected) {
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
#if defined(Q_OS_WIN)
    const QString extension = QStringLiteral(".dll");
#elif defined(Q_OS_MACOS)
    const QString extension = QStringLiteral(".dylib");
#else
    const QString extension = QStringLiteral(".so");
#endif

    writeLibrary(temp.path(), extension);
    writeManifest(temp.path(), QStringLiteral("plugin") + extension,
                  QStringLiteral(",\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\""));

    QStringList diagnostics;
    const auto plugins = ccos::plugins::PluginManager::scan({temp.path()}, &diagnostics);
    EXPECT_TRUE(plugins.isEmpty());
    EXPECT_FALSE(diagnostics.isEmpty());
}
