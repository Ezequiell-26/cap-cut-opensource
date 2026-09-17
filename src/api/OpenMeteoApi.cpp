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

namespace ccos::api {

WeatherData OpenMeteoApi::getCurrentWeather(double latitude, double longitude) {
    WeatherData result;
    
    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(latitude));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(longitude));
    query.addQueryItem(QStringLiteral("current_weather"), QStringLiteral("true"));
    query.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(15000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply] { reply->abort(); loop.quit(); });
    loop.exec();
    
    if (timeout.isActive()) timeout.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return result;
    }
    
    const QByteArray payload = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return result;
    }
    
    const auto root = doc.object();
    const auto current = root.value(QStringLiteral("current_weather")).toObject();
    
    result.temperature = current.value(QStringLiteral("temperature")).toDouble();
    result.windSpeed = current.value(QStringLiteral("windspeed")).toDouble();
    result.humidity = 0; // Open-Meteo free API doesn't include humidity in current_weather
    
    int weatherCode = current.value(QStringLiteral("weathercode")).toInt(0);
    switch (weatherCode) {
        case 0: result.condition = QStringLiteral("Clear sky"); break;
        case 1: case 2: case 3: result.condition = QStringLiteral("Partly cloudy"); break;
        case 45: case 48: result.condition = QStringLiteral("Foggy"); break;
        case 51: case 53: case 55: case 56: case 57: result.condition = QStringLiteral("Drizzle"); break;
        case 61: case 63: case 65: case 66: case 67: result.condition = QStringLiteral("Rainy"); break;
        case 71: case 73: case 75: case 77: result.condition = QStringLiteral("Snowy"); break;
        case 80: case 81: case 82: result.condition = QStringLiteral("Rain showers"); break;
        case 85: case 86: result.condition = QStringLiteral("Snow showers"); break;
        case 95: case 96: case 99: result.condition = QStringLiteral("Thunderstorm"); break;
        default: result.condition = QStringLiteral("Unknown"); break;
    }
    
    result.iconUrl = QStringLiteral("https://open-meteo.com/static/icons/%1.svg").arg(weatherCode);
    
    return result;
}

QVector<QVariantMap> OpenMeteoApi::getForecast(double latitude, double longitude, int days) {
    QVector<QVariantMap> forecast;
    
    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(latitude));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(longitude));
    query.addQueryItem(QStringLiteral("daily"), QStringLiteral("weathercode,temperature_2m_max,temperature_2m_min,precipitation_probability_max,windspeed_10m_max"));
    query.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    query.addQueryItem(QStringLiteral("forecast_days"), QString::number(qBound(1, days, 16)));
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(15000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply] { reply->abort(); loop.quit(); });
    loop.exec();
    
    if (timeout.isActive()) timeout.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return forecast;
    }
    
    const QByteArray payload = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return forecast;
    }
    
    const auto root = doc.object();
    const auto daily = root.value(QStringLiteral("daily")).toObject();
    
    const auto timeArr = daily.value(QStringLiteral("time")).toArray();
    const auto tempMaxArr = daily.value(QStringLiteral("temperature_2m_max")).toArray();
    const auto tempMinArr = daily.value(QStringLiteral("temperature_2m_min")).toArray();
    const auto precipArr = daily.value(QStringLiteral("precipitation_probability_max")).toArray();
    const auto windArr = daily.value(QStringLiteral("windspeed_10m_max")).toArray();
    const auto weatherArr = daily.value(QStringLiteral("weathercode")).toArray();
    
    for (int i = 0; i < timeArr.size() && i < 16; ++i) {
        QVariantMap day;
        day.insert(QStringLiteral("date"), timeArr.at(i).toString());
        day.insert(QStringLiteral("tempMax"), tempMaxArr.at(i).toDouble());
        day.insert(QStringLiteral("tempMin"), tempMinArr.at(i).toDouble());
        day.insert(QStringLiteral("precipitationChance"), precipArr.at(i).toInt());
        day.insert(QStringLiteral("windSpeed"), windArr.at(i).toDouble());
        
        int weatherCode = weatherArr.at(i).toInt(0);
        QString condition;
        switch (weatherCode) {
            case 0: condition = QStringLiteral("Clear sky"); break;
            case 1: case 2: case 3: condition = QStringLiteral("Partly cloudy"); break;
            case 45: case 48: condition = QStringLiteral("Foggy"); break;
            case 51: case 53: case 55: case 56: case 57: condition = QStringLiteral("Drizzle"); break;
            case 61: case 63: case 65: case 66: case 67: condition = QStringLiteral("Rainy"); break;
            case 71: case 73: case 75: case 77: condition = QStringLiteral("Snowy"); break;
            case 80: case 81: case 82: condition = QStringLiteral("Rain showers"); break;
            case 85: case 86: condition = QStringLiteral("Snow showers"); break;
            case 95: case 96: case 99: condition = QStringLiteral("Thunderstorm"); break;
            default: condition = QStringLiteral("Unknown"); break;
        }
        day.insert(QStringLiteral("condition"), condition);
        
        forecast.append(day);
    }
    
    return forecast;
}

} // namespace ccos::api
