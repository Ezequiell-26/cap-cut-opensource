#include "api/NasaMediaApi.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <gtest/gtest.h>

TEST(NasaMediaApiTest, ParsesImageApodMetadata) {
    const QJsonObject object{
        {QStringLiteral("date"), QStringLiteral("2026-09-17")},
        {QStringLiteral("title"), QStringLiteral("Test Nebula")},
        {QStringLiteral("copyright"), QStringLiteral("CCOS Test")},
        {QStringLiteral("media_type"), QStringLiteral("image")},
        {QStringLiteral("url"), QStringLiteral("https://example.invalid/apod.jpg")},
        {QStringLiteral("hdurl"), QStringLiteral("https://example.invalid/apod-hd.jpg")}
    };

    const auto item = ccos::api::NasaMediaApi::parseApod(QJsonDocument(object));
    EXPECT_EQ(item.provider, QStringLiteral("nasa_apod"));
    EXPECT_EQ(item.id, QStringLiteral("2026-09-17"));
    EXPECT_EQ(item.title, QStringLiteral("Test Nebula"));
    EXPECT_EQ(item.creator, QStringLiteral("CCOS Test"));
    EXPECT_EQ(item.downloadUrl, QStringLiteral("https://example.invalid/apod-hd.jpg"));
    EXPECT_TRUE(item.isUsable());
}

TEST(NasaMediaApiTest, RejectsUnsupportedMediaType) {
    const QJsonObject object{
        {QStringLiteral("date"), QStringLiteral("2026-09-17")},
        {QStringLiteral("media_type"), QStringLiteral("audio")},
        {QStringLiteral("url"), QStringLiteral("https://example.invalid/audio")}
    };

    const auto item = ccos::api::NasaMediaApi::parseApod(QJsonDocument(object));
    EXPECT_FALSE(item.isUsable());
}
