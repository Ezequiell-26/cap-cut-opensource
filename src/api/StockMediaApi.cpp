#include "api/StockMediaApi.hpp"
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>

namespace ccos::api {

QStringList StockMediaApi::supportedProviders() {
    return {QStringLiteral("pexels"), QStringLiteral("pixabay")};
}

StockMediaApi::StockMediaApi(QString apiKey) : apiKey_(std::move(apiKey)) {}

void StockMediaApi::setProvider(const QString& provider) {
    if (supportedProviders().contains(provider)) {
        provider_ = provider;
    }
}

QVector<StockMediaResult> StockMediaApi::searchVideos(const QString& query, int page, int perPage) {
    if (provider_ == QStringLiteral("pexels")) {
        return searchPexels(query, QStringLiteral("video"), page, perPage);
    } else if (provider_ == QStringLiteral("pixabay")) {
        return searchPixabay(query, QStringLiteral("videos"), page, perPage);
    }
    return {};
}

QVector<StockMediaResult> StockMediaApi::searchImages(const QString& query, int page, int perPage) {
    if (provider_ == QStringLiteral("pexels")) {
        return searchPexels(query, QStringLiteral("photo"), page, perPage);
    } else if (provider_ == QStringLiteral("pixabay")) {
        return searchPixabay(query, QStringLiteral("images"), page, perPage);
    }
    return {};
}

QVector<StockMediaResult> StockMediaApi::searchPexels(const QString& query, const QString& type, int page, int perPage) {
    QVector<StockMediaResult> results;
    if (apiKey_.isEmpty()) {
        return results;
    }
    
    QString endpoint;
    if (type == QStringLiteral("video")) {
        endpoint = QStringLiteral("https://api.pexels.com/videos/search");
    } else {
        endpoint = QStringLiteral("https://api.pexels.com/v1/search");
    }
    
    QUrl url(endpoint);
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem(QStringLiteral("query"), query);
    queryBuilder.addQueryItem(QStringLiteral("page"), QString::number(page));
    queryBuilder.addQueryItem(QStringLiteral("per_page"), QString::number(perPage));
    queryBuilder.addQueryItem(QStringLiteral("orientation"), QStringLiteral("landscape"));
    url.setQuery(queryBuilder);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", apiKey_.toUtf8());
    
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(30000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply] { reply->abort(); loop.quit(); });
    loop.exec();
    
    if (timeout.isActive()) timeout.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return results;
    }
    
    const QByteArray payload = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return results;
    }
    
    const auto root = doc.object();
    const auto videos = root.value(type == QStringLiteral("video") ? QStringLiteral("videos") : QStringLiteral("photos")).toArray();
    
    for (const auto& item : videos) {
        const auto obj = item.toObject();
        StockMediaResult result;
        result.id = QString::number(obj.value(QStringLiteral("id")).toInt());
        result.type = type == QStringLiteral("video") ? QStringLiteral("video") : QStringLiteral("image");
        result.author = obj.value(QStringLiteral("photographer")).toString(obj.value(QStringLiteral("user")).toObject().value(QStringLiteral("name")).toString());
        result.license = QStringLiteral("Pexels License (Free to use)");
        
        if (result.type == QStringLiteral("video")) {
            const auto videoFiles = obj.value(QStringLiteral("video_files")).toArray();
            if (!videoFiles.isEmpty()) {
                for (const auto& vf : videoFiles) {
                    const auto vfo = vf.toObject();
                    const QString quality = vfo.value(QStringLiteral("quality")).toString();
                    if (quality == QStringLiteral("hd") || quality == QStringLiteral("sd")) {
                        result.downloadUrl = vfo.value(QStringLiteral("link")).toString();
                        break;
                    }
                }
            }
            if (result.downloadUrl.isEmpty() && !videoFiles.isEmpty()) {
                result.downloadUrl = videoFiles.first().toObject().value(QStringLiteral("link")).toString();
            }
            const auto image = obj.value(QStringLiteral("image")).toString();
            result.thumbnailUrl = obj.value(QStringLiteral("thumbnail")).toString(image);
            result.url = obj.value(QStringLiteral("url")).toString();
            result.width = obj.value(QStringLiteral("width")).toInt();
            result.height = obj.value(QStringLiteral("height")).toInt();
            result.durationSec = obj.value(QStringLiteral("duration")).toDouble() / 1000.0;
        } else {
            const auto src = obj.value(QStringLiteral("src")).toObject();
            result.url = src.value(QStringLiteral("large2x")).toString(src.value(QStringLiteral("large")).toString(src.value(QStringLiteral("original")).toString()));
            result.thumbnailUrl = src.value(QStringLiteral("thumbnail")).toString();
            result.downloadUrl = src.value(QStringLiteral("original")).toString();
            result.width = obj.value(QStringLiteral("width")).toInt();
            result.height = obj.value(QStringLiteral("height")).toInt();
        }
        
        if (!result.downloadUrl.isEmpty()) {
            results.append(result);
        }
    }
    
    return results;
}

QVector<StockMediaResult> StockMediaApi::searchPixabay(const QString& query, const QString& type, int page, int perPage) {
    QVector<StockMediaResult> results;
    if (apiKey_.isEmpty()) {
        return results;
    }
    
    QString endpoint;
    if (type == QStringLiteral("videos")) {
        endpoint = QStringLiteral("https://pixabay.com/api/videos/");
    } else {
        endpoint = QStringLiteral("https://pixabay.com/api/");
    }
    
    QUrl url(endpoint);
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem(QStringLiteral("key"), apiKey_);
    queryBuilder.addQueryItem(QStringLiteral("q"), query);
    queryBuilder.addQueryItem(QStringLiteral("page"), QString::number(page));
    queryBuilder.addQueryItem(QStringLiteral("per_page"), QString::number(perPage));
    queryBuilder.addQueryItem(QStringLiteral("image_type"), type == QStringLiteral("videos") ? QStringLiteral("all") : QStringLiteral("photo"));
    queryBuilder.addQueryItem(QStringLiteral("orientation"), QStringLiteral("landscape"));
    url.setQuery(queryBuilder);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(30000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply] { reply->abort(); loop.quit(); });
    loop.exec();
    
    if (timeout.isActive()) timeout.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return results;
    }
    
    const QByteArray payload = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return results;
    }
    
    const auto root = doc.object();
    const auto hits = root.value(QStringLiteral("hits")).toArray();
    
    for (const auto& item : hits) {
        const auto obj = item.toObject();
        StockMediaResult result;
        result.id = QString::number(obj.value(QStringLiteral("id")).toInt());
        result.type = type == QStringLiteral("videos") ? QStringLiteral("video") : QStringLiteral("image");
        result.author = obj.value(QStringLiteral("user")).toString();
        result.license = QStringLiteral("Pixabay License (Free to use)");
        
        if (result.type == QStringLiteral("video")) {
            result.url = obj.value(QStringLiteral("url")).toString();
            result.thumbnailUrl = obj.value(QStringLiteral("thumbnails")).toObject().value(QStringLiteral("large")).toString();
            result.downloadUrl = obj.value(QStringLiteral("videos")).toObject().value(QStringLiteral("hd")).toObject().value(QStringLiteral("url")).toString();
            if (result.downloadUrl.isEmpty()) {
                result.downloadUrl = obj.value(QStringLiteral("videos")).toObject().value(QStringLiteral("sd")).toObject().value(QStringLiteral("url")).toString();
            }
            result.width = obj.value(QStringLiteral("picture_width")).toInt();
            result.height = obj.value(QStringLiteral("picture_height")).toInt();
            result.durationSec = obj.value(QStringLiteral("duration")).toDouble();
        } else {
            result.url = obj.value(QStringLiteral("large2xURL")).toString(obj.value(QStringLiteral("largeURL")).toString(obj.value(QStringLiteral("webformatURL")).toString()));
            result.thumbnailUrl = obj.value(QStringLiteral("previewURL")).toString();
            result.downloadUrl = obj.value(QStringLiteral("imageURL")).toString(obj.value(QStringLiteral("large2xURL")).toString());
            result.width = obj.value(QStringLiteral("imageWidth")).toInt();
            result.height = obj.value(QStringLiteral("imageHeight")).toInt();
        }
        
        if (!result.downloadUrl.isEmpty()) {
            results.append(result);
        }
    }
    
    return results;
}

QString StockMediaApi::getDownloadUrl(const StockMediaResult& result) {
    return result.downloadUrl;
}

} // namespace ccos::api
