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

    constexpr qint64 kMaxResponseBytes = 2 * 1024 * 1024;
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.9 open-meteo-client"));
    request.setRawHeader("Accept", "application/json");
    QNetworkReply* reply = manager.get(request);

    const QVariant length = reply->header(QNetworkRequest::ContentLengthHeader);
    if (length.isValid() && length.toLongLong() > kMaxResponseBytes) {
        reply->abort();
    }

    QByteArray payload;
    bool oversized = false;
    QObject::connect(reply, &QNetworkReply::readyRead, reply, [&]() {
        if (oversized) return;
        payload.append(reply->readAll());
        if (payload.size() > kMaxResponseBytes) {
            oversized = true;
            reply->abort();
        }
    });

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
    payload.append(reply->readAll());
    const bool ok = !oversized &&
                    payload.size() <= kMaxResponseBytes &&
                    reply->error() == QNetworkReply::NoError;
    reply->deleteLater();
    if (!ok) return false;

    QJsonParseError parseError{};
    const QJsonDocument parsed = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError) return false;
    *document = parsed;
    return true;
}

QString normalizeOptional(const QJsonObject& object, const QString& key) {
    return object.value(key).toString().trimmed();
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

QVector<GeocodedLocation> OpenMeteoApi::parseGeocoding(const QJsonDocument& document) {
    QVector<GeocodedLocation> results;
    if (!document.isObject()) return results;

    const auto values = document.object().value(QStringLiteral("results")).toArray();
    results.reserve(static_cast<int>(std::min<qsizetype>(values.size(), 100)));
    for (qsizetype i = 0; i < values.size() && i < 100; ++i) {
        const auto object = values.at(i).toObject();
        GeocodedLocation result;
        result.name = object.value(QStringLiteral("name")).toString().trimmed();
        result.country = object.value(QStringLiteral("country")).toString().trimmed();
        result.countryCode = object.value(QStringLiteral("country_code")).toString().trimmed().toUpper();
        result.admin1 = object.value(QStringLiteral("admin1")).toString().trimmed();
        result.timezone = object.value(QStringLiteral("timezone")).toString().trimmed();
        result.latitude = object.value(QStringLiteral("latitude")).toDouble();
        result.longitude = object.value(QStringLiteral("longitude")).toDouble();
        if (result.name.isEmpty()) continue;
        if (result.latitude < -90.0 || result.latitude > 90.0 ||
            result.longitude < -180.0 || result.longitude > 180.0) continue;
        results.append(result);
    }
    return results;
}

QVector<GeocodedLocation> OpenMeteoApi::searchLocations(const QString& name, int count) {
    const QString query = name.trimmed();
    if (query.isEmpty()) return {};

    QUrl url(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("name"), query);
    params.addQueryItem(QStringLiteral("count"), QString::number(qBound(1, count, 100)));
    params.addQueryItem(QStringLiteral("language"), QStringLiteral("en"));
    params.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(params);

    QJsonDocument document;
    if (!getJson(url, &document)) return {};
    return parseGeocoding(document);
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
