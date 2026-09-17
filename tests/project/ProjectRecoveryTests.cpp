#include "project/ProjectRecovery.hpp"

#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

namespace {
ccos::project::Project makeProject(const QString& name) {
    return ccos::project::Project(name);
}
}

TEST(ProjectRecoveryTests, UsesProjectUuidToSeparateSnapshots) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::project::ProjectRecoveryManager manager(dir.path());

    const auto first = makeProject(QStringLiteral("First"));
    const auto second = makeProject(QStringLiteral("Second"));

    const QString firstPath = manager.pathFor(first);
    const QString secondPath = manager.pathFor(second);
    EXPECT_FALSE(firstPath.isEmpty());
    EXPECT_FALSE(secondPath.isEmpty());
    EXPECT_NE(firstPath, secondPath);
}

TEST(ProjectRecoveryTests, SaveLatestLoadAndRemoveAreDeterministic) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::project::ProjectRecoveryManager manager(dir.path());

    auto project = makeProject(QStringLiteral("Recover Me"));
    QString error;
    ASSERT_TRUE(manager.save(project, &error)) << error.toStdString();

    const QString snapshot = manager.latestSnapshotPath();
    ASSERT_FALSE(snapshot.isEmpty());
    EXPECT_TRUE(QFile::exists(snapshot));

    ccos::project::Project loaded;
    ASSERT_TRUE(manager.load(snapshot, &loaded, &error)) << error.toStdString();
    EXPECT_EQ(loaded.name(), QStringLiteral("Recover Me"));
    EXPECT_EQ(loaded.id().toString(), project.id().toString());

    EXPECT_TRUE(manager.remove(project));
    EXPECT_TRUE(manager.latestSnapshotPath().isEmpty());
    EXPECT_TRUE(manager.remove(project));
}

TEST(ProjectRecoveryTests, RejectsInvalidLoadDestination) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::project::ProjectRecoveryManager manager(dir.path());

    QString error;
    EXPECT_FALSE(manager.load(QStringLiteral("/missing/recovery.ccos"), nullptr, &error));
    EXPECT_NE(error.indexOf(QStringLiteral("required")), -1);
}
