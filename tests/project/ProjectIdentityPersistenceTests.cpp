#include "project/Project.hpp"
#include "project/ProjectSerializer.hpp"

#include <gtest/gtest.h>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

namespace {

QString validUuid1() { return QStringLiteral("11111111-1111-4111-8111-111111111111"); }
QString validUuid2() { return QStringLiteral("22222222-2222-4222-8222-222222222222"); }
QString validUuid3() { return QStringLiteral("33333333-3333-4333-8333-333333333333"); }

}

TEST(ProjectIdentityPersistenceTests, PreservesProjectAssetClipAndTextIds) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    ccos::project::Project source(
        ccos::core::Uuid(validUuid1().toStdString()), QStringLiteral("Identity Test"));
    ccos::media::MediaAsset asset(
        ccos::core::Uuid(validUuid2().toStdString()), QStringLiteral("/tmp/example.mp4"));
    asset.setName(QStringLiteral("Custom Asset Name"));
    source.addAsset(asset);

    ccos::timeline::Clip clip(
        ccos::core::Uuid(validUuid3().toStdString()), source.assets().front().id());
    source.timeline().addClipToVideo(clip);

    ccos::text::TextLayer text(
        ccos::core::Uuid(QStringLiteral("44444444-4444-4444-8444-444444444444").toStdString()),
        QStringLiteral("Hello"));
    source.addTextLayer(text);

    const QString path = dir.filePath(QStringLiteral("identity.ccos"));
    QString error;
    ASSERT_TRUE(ccos::project::ProjectSerializer::save(source, path, &error)) << error.toStdString();

    ccos::project::Project loaded;
    ASSERT_TRUE(ccos::project::ProjectSerializer::load(loaded, path, &error)) << error.toStdString();

    EXPECT_EQ(loaded.id().toString(), source.id().toString());
    ASSERT_EQ(loaded.assets().size(), 1U);
    EXPECT_EQ(loaded.assets().front().id().toString(), source.assets().front().id().toString());
    EXPECT_EQ(loaded.assets().front().name(), QStringLiteral("Custom Asset Name"));

    ASSERT_FALSE(loaded.timeline().tracks().empty());
    ASSERT_FALSE(loaded.timeline().tracks().front().clips().empty());
    EXPECT_EQ(loaded.timeline().tracks().front().clips().front().id().toString(),
              clip.id().toString());

    ASSERT_EQ(loaded.textLayers().size(), 1U);
    EXPECT_EQ(loaded.textLayers().front().id().toString(), text.id().toString());
}

TEST(ProjectIdentityPersistenceTests, RejectsDuplicateAssetIds) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    const QString duplicateId = validUuid2();
    QJsonObject root{
        {QStringLiteral("format"), QStringLiteral("ccos.project")},
        {QStringLiteral("version"), 6},
        {QStringLiteral("id"), validUuid1()},
        {QStringLiteral("name"), QStringLiteral("Corrupt")}
    };
    root[QStringLiteral("assets")] = QJsonArray{
        QJsonObject{{QStringLiteral("id"), duplicateId}, {QStringLiteral("path"), QStringLiteral("a.mp4")} },
        QJsonObject{{QStringLiteral("id"), duplicateId}, {QStringLiteral("path"), QStringLiteral("b.mp4")} }
    };

    const QString path = dir.filePath(QStringLiteral("duplicate.ccos"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_GT(file.write(QJsonDocument(root).toJson()), 0);
    file.close();

    ccos::project::Project loaded;
    QString error;
    EXPECT_FALSE(ccos::project::ProjectSerializer::load(loaded, path, &error));
    EXPECT_NE(error.indexOf(QStringLiteral("Duplicate asset UUID")), -1);
}
