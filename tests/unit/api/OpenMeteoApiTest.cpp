#include "api/OpenMeteoApi.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <gtest/gtest.h>

TEST(OpenMeteoApiTest, ParsesAndBoundsGeocodedLocations) {
    const QJsonObject object{
        {QStringLiteral("results"), QJsonArray{
            QJsonObject{
                {QStringLiteral("name"), QStringLiteral("Buenos Aires")},
                {QStringLiteral("country"), QStringLiteral("Argentina")},
                {QStringLiteral("country_code"), QStringLiteral("AR")},
                {QStringLiteral("admin1"), QStringLiteral("Buenos Aires")},
                {QStringLiteral("timezone"), QStringLiteral("America/Argentina/Buenos_Aires")},
                {QStringLiteral("latitude"), -34.6037},
                {QStringLiteral("longitude"), -58.3816}
            },
            QJsonObject{
                {QStringLiteral("name"), QStringLiteral("Invalid")},
                {QStringLiteral("latitude"), 999.0},
                {QStringLiteral("longitude"), 0.0}
            }
        }}
    };

    const auto results = ccos::api::OpenMeteoApi::parseGeocoding(QJsonDocument(object));
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.front().name, QStringLiteral("Buenos Aires"));
    EXPECT_EQ(results.front().countryCode, QStringLiteral("AR"));
    EXPECT_EQ(results.front().timezone, QStringLiteral("America/Argentina/Buenos_Aires"));
    EXPECT_DOUBLE_EQ(results.front().latitude, -34.6037);
    EXPECT_DOUBLE_EQ(results.front().longitude, -58.3816);
}

TEST(OpenMeteoApiTest, RejectsNonObjectDocument) {
    const auto results = ccos::api::OpenMeteoApi::parseGeocoding(QJsonDocument::fromJson("[]"));
    EXPECT_TRUE(results.isEmpty());
}
