#pragma once
#include <QString>

class QJsonDocument;
#include <QVariantMap>
#include <QVector>

namespace ccos::api {

struct GeocodedLocation {
    QString name;
    QString country;
    QString countryCode;
    QString admin1;
    QString timezone;
    double latitude = 0.0;
    double longitude = 0.0;
};

struct WeatherData {
    QString location;
    double temperature = 0.0;
    QString condition;
    int humidity = 0;
    double windSpeed = 0.0;
    QString iconUrl;
};

class OpenMeteoApi {
public:
    static WeatherData getCurrentWeather(double latitude, double longitude);
    static QVector<GeocodedLocation> searchLocations(const QString& name, int count = 10);
    static QVector<GeocodedLocation> parseGeocoding(const QJsonDocument& document);
    static QVector<QVariantMap> getForecast(double latitude, double longitude, int days = 7);
};

} // namespace ccos::api
