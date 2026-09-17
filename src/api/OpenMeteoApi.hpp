#pragma once
#include <QString>
#include <QVariantMap>

namespace ccos::api {

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
    static QVector<QVariantMap> getForecast(double latitude, double longitude, int days = 7);
};

} // namespace ccos::api
