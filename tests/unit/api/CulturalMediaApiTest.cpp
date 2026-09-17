#include "api/CulturalMediaApi.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

#include <gtest/gtest.h>

namespace {

QJsonDocument json(const char* value) {
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray(value), &error);
    EXPECT_EQ(error.error, QJsonParseError::NoError);
    return document;
}

} // namespace

TEST(CulturalMediaApiTest, ListsAllConfiguredProviders) {
    const QStringList providers = ccos::api::CulturalMediaApi::supportedProviders();
    EXPECT_TRUE(providers.contains(QStringLiteral("internet_archive")));
    EXPECT_TRUE(providers.contains(QStringLiteral("smithsonian_open_access")));
    EXPECT_TRUE(providers.contains(QStringLiteral("met_open_access")));
    EXPECT_TRUE(providers.contains(QStringLiteral("europeana")));
    EXPECT_TRUE(providers.contains(QStringLiteral("library_of_congress")));
}

TEST(CulturalMediaApiTest, ParsesInternetArchiveItems) {
    const auto results = ccos::api::CulturalMediaApi::parseInternetArchive(json(R"json({
        "response": {"docs": [{
            "identifier": "ccos-test-asset",
            "title": "Test archive asset",
            "creator": "Test Creator",
            "mediatype": "movies"
        }]}
    })json"));

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().provider, QStringLiteral("internet_archive"));
    EXPECT_EQ(results.first().id, QStringLiteral("ccos-test-asset"));
    EXPECT_FALSE(results.first().downloadUrl.isEmpty());
}

TEST(CulturalMediaApiTest, ParsesSmithsonianOpenAccessMedia) {
    const auto results = ccos::api::CulturalMediaApi::parseSmithsonian(json(R"json({
        "response": {"rows": [{"content": {
            "descriptiveNonRepeating": {
                "record_ID": "TEST-1",
                "title": {"content": "Museum test"},
                "recordLink": "https://example.invalid/item/TEST-1",
                "online_media": {"media": [{"content": "https://example.invalid/test.jpg"}]}
            }
        }}]}
    })json"));

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().provider, QStringLiteral("smithsonian_open_access"));
    EXPECT_EQ(results.first().title, QStringLiteral("Museum test"));
    EXPECT_EQ(results.first().downloadUrl, QStringLiteral("https://example.invalid/test.jpg"));
}

TEST(CulturalMediaApiTest, ParsesMetObject) {
    const auto item = ccos::api::CulturalMediaApi::parseMetObject(json(R"json({
        "objectID": 123,
        "title": "Open Access Artwork",
        "artistDisplayName": "Artist",
        "objectURL": "https://www.metmuseum.org/art/collection/search/123",
        "primaryImageSmall": "https://example.invalid/small.jpg",
        "primaryImage": "https://example.invalid/full.jpg"
    })json"));

    EXPECT_EQ(item.provider, QStringLiteral("met_open_access"));
    EXPECT_EQ(item.id, QStringLiteral("123"));
    EXPECT_EQ(item.title, QStringLiteral("Open Access Artwork"));
    EXPECT_EQ(item.downloadUrl, QStringLiteral("https://example.invalid/full.jpg"));
}

TEST(CulturalMediaApiTest, ParsesEuropeanaRightsAndPreview) {
    const auto results = ccos::api::CulturalMediaApi::parseEuropeana(json(R"json({
        "items": [{
            "id": "/item/test/1",
            "title": "European collection item",
            "dcCreator": "Creator",
            "rights": "http://creativecommons.org/publicdomain/mark/1.0/",
            "edmIsShownAt": "https://example.invalid/item/1",
            "edmPreview": "https://example.invalid/preview.jpg"
        }]
    })json"));

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().license, QStringLiteral("http://creativecommons.org/publicdomain/mark/1.0/"));
    EXPECT_EQ(results.first().downloadUrl, QStringLiteral("https://example.invalid/preview.jpg"));
}

TEST(CulturalMediaApiTest, ParsesLibraryOfCongressImages) {
    const auto results = ccos::api::CulturalMediaApi::parseLibraryOfCongress(json(R"json({
        "results": [{
            "id": "https://www.loc.gov/item/test/",
            "title": "LOC test",
            "contributor": "Contributor",
            "image_url": ["https://example.invalid/loc.jpg"]
        }]
    })json"));

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().provider, QStringLiteral("library_of_congress"));
    EXPECT_EQ(results.first().downloadUrl, QStringLiteral("https://example.invalid/loc.jpg"));
}

TEST(CulturalMediaApiTest, RejectsMalformedDocuments) {
    EXPECT_TRUE(ccos::api::CulturalMediaApi::parseInternetArchive(QJsonDocument{}).isEmpty());
    EXPECT_TRUE(ccos::api::CulturalMediaApi::parseSmithsonian(QJsonDocument{}).isEmpty());
    EXPECT_TRUE(ccos::api::CulturalMediaApi::parseEuropeana(QJsonDocument{}).isEmpty());
    EXPECT_TRUE(ccos::api::CulturalMediaApi::parseLibraryOfCongress(QJsonDocument{}).isEmpty());
    EXPECT_FALSE(ccos::api::CulturalMediaApi::parseMetObject(QJsonDocument{}).isUsable());
}
