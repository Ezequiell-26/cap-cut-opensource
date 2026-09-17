#include "api/OpenMeteoApi.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace ccos::api {
namespace {

bool getJson(const QUrl& url, QJsonDocument* document) {
    if (!document) return false;

    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(QNetworkRequest(url));
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(15'000);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply]() {
        if (reply->isRunning()) reply->abort();
        loop.quit();
    });
    loop.exec();

    if (timeout.isActive()) timeout.stop();
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument parsed = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();
    if (parseError.error != QJsonParseError::NoError) return false;
    *document = parsed;
    return true;
}

QString conditionForCode(int code) {
    switch (code) {
        case 0: return QStringLiteral("Clear sky");
        case 1: case 2: case 3: return QStringLiteral("Partly cloudy");
        case 45: case 48: return QStringLiteral("Foggy");
        case 51: case 53: case 55: case 56: case 57: return QStringLiteral("Drizzle");
        case 61: case 63: case 65: case 66: case 67: return QStringLiteral("Rainy");
        case 71: case 73: case 75: case 77: return QStringLiteral("Snowy");
        case 80: case 81: case 82: return QStringLiteral("Rain showers");
        case 85: case 86: return QStringLiteral("Snow showers");
        case 95: case 96: case 99: return QStringLiteral("Thunderstorm");
        default: return QStringLiteral("Unknown");
    }
}

}

WeatherData OpenMeteoApi::getCurrentWeather(double latitude, double longitude) {
    WeatherData result;
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) return result;

    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(latitude, 'f', 6));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(longitude, 'f', 6));
    query.addQueryItem(QStringLiteral("current"), QStringLiteral("temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code"));
    query.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    url.setQuery(query);

    QJsonDocument document;
    if (!getJson(url, &document) || !document.isObject()) return result;
    const QJsonObject current = document.object().value(QStringLiteral("current")).toObject();
    result.temperature = current.value(QStringLiteral("temperature_2m")).toDouble();
    result.windSpeed = current.value(QStringLiteral("wind_speed_10m")).toDouble();
    result.humidity = current.value(QStringLiteral("relative_humidity_2m")).toDouble();
    const int code = current.value(QStringLiteral("weather_code")).toInt(-1);
    result.condition = conditionForCode(code);
    result.iconUrl = QStringLiteral("https://open-meteo.com/static/icons/%1.svg").arg(code);
    return result;
}

QVector<QVariantMap> OpenMeteoApi::getForecast(double latitude, double longitude, int days) {
    QVector<QVariantMap> forecast;
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) return forecast;

    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(latitude, 'f', 6));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(longitude, 'f', 6));
    query.addQueryItem(QStringLiteral("daily"), QStringLiteral("weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,wind_speed_10m_max"));
    query.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    query.addQueryItem(QStringLiteral("forecast_days"), QString::number(qBound(1, days, 16)));
    url.setQuery(query);

    QJsonDocument document;
    if (!getJson(url, &document) || !document.isObject()) return forecast;
    const QJsonObject daily = document.object().value(QStringLiteral("daily")).toObject();
    const QJsonArray times = daily.value(QStringLiteral("time")).toArray();
    const QJsonArray maxTemps = daily.value(QStringLiteral("temperature_2m_max")).toArray();
    const QJsonArray minTemps = daily.value(QStringLiteral("temperature_2m_min")).toArray();
    const QJsonArray precip = daily.value(QStringLiteral("precipitation_probability_max")).toArray();
    const QJsonArray winds = daily.value(QStringLiteral("wind_speed_10m_max")).toArray();
    const QJsonArray codes = daily.value(QStringLiteral("weather_code")).toArray();

    const qsizetype count = std::min({times.size(), maxTemps.size(), minTemps.size(), precip.size(), winds.size(), codes.size(), qsizetype(16)});
    forecast.reserve(static_cast<int>(count));
    for (qsizetype i = 0; i < count; ++i) {
        const int code = codes.at(i).toInt(-1);
        QVariantMap day;
        day.insert(QStringLiteral("date"), times.at(i).toString());
        day.insert(QStringLiteral("tempMax"), maxTemps.at(i).toDouble());
        day.insert(QStringLiteral("tempMin"), minTemps.at(i).toDouble());
        day.insert(QStringLiteral("precipitationChance"), precip.at(i).toInt());
        day.insert(QStringLiteral("windSpeed"), winds.at(i).toDouble());
        day.insert(QStringLiteral("condition"), conditionForCode(code));
        day.insert(QStringLiteral("weatherCode"), code);
        forecast.append(day);
    }
    return forecast;
}

} // namespace ccos::api
