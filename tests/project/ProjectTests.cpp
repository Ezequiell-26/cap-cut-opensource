#include "project/Project.hpp"
#include "project/ProjectSerializer.hpp"
#include <gtest/gtest.h>
#include <QTemporaryDir>

TEST(ProjectTests, SavesAndLoadsProject) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::project::Project source(QStringLiteral("Demo"));
    source.addAsset(ccos::media::MediaAsset(QStringLiteral("/tmp/demo.mp4")));

    const auto path = dir.filePath(QStringLiteral("demo.ccos"));
    QString error;
    ASSERT_TRUE(ccos::project::ProjectSerializer::save(source, path, &error)) << error.toStdString();

    ccos::project::Project loaded;
    ASSERT_TRUE(ccos::project::ProjectSerializer::load(loaded, path, &error)) << error.toStdString();
    EXPECT_EQ(loaded.name(), QStringLiteral("Demo"));
    ASSERT_EQ(loaded.assets().size(), 1U);
    EXPECT_EQ(loaded.assets().front().path(), QStringLiteral("/tmp/demo.mp4"));
}
