#include "PremiumStockApi.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>

namespace ccos::api {
namespace {

QNetworkRequest requestFor(const QUrl& url, const QString& userAgent = QStringLiteral("CCOS-Editor/0.8")) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setRawHeader("Accept", "application/json");
    return request;
}

QVector<StockVideo> parseVideoArray(const QJsonArray& array, const QString& license) {
    QVector<StockVideo> results;
    results.reserve(array.size());
    for (const QJsonValue& value : array) {
        const QJsonObject item = value.toObject();
        StockVideo video;
        video.id = item.value(QStringLiteral("id")).toVariant().toString();
        if (video.id.isEmpty()) video.id = item.value(QStringLiteral("slug")).toString();
        video.title = item.value(QStringLiteral("title")).toString();
        video.url = item.value(QStringLiteral("url")).toString();
        if (video.url.isEmpty()) video.url = item.value(QStringLiteral("download_url")).toString();
        video.thumbnailUrl = item.value(QStringLiteral("thumbnail")).toString();
        video.duration = item.value(QStringLiteral("duration")).toString();
        video.resolution = item.value(QStringLiteral("resolution")).toString();
        video.author = item.value(QStringLiteral("author")).toString();
        video.license = license;
        if (!video.id.isEmpty()) results.append(video);
    }
    return results;
}

void unsupported(const QString& provider, QObject* object,
                 std::function<void(const QVector<StockVideo>&)> callback) {
    emit static_cast<PremiumStockApi*>(object)->errorOccurred(
        QStringLiteral("%1 adapter requires a currently configured public API endpoint or credential").arg(provider));
    if (callback) callback({});
}

}

PremiumStockApi::PremiumStockApi(QObject* parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {}

PremiumStockApi::~PremiumStockApi() = default;

void PremiumStockApi::searchVideos(const QString& query, int page,
                                   std::function<void(const QVector<StockVideo>&)> callback) {
    QUrl url(QStringLiteral("https://mixkit.co/api/videos"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("search"), query.trimmed());
    params.addQueryItem(QStringLiteral("page"), QString::number(std::max(1, page)));
    url.setQuery(params);
    auto* reply = m_networkManager->get(requestFor(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() mutable {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
        } else {
            parseMixkitVideos(reply->readAll(), std::move(callback));
        }
        reply->deleteLater();
    });
}

void PremiumStockApi::searchMusic(const QString& genre,
                                  std::function<void(const QVector<StockMusic>&)> callback) {
    QUrl url(QStringLiteral("https://mixkit.co/api/music"));
    QUrlQuery params;
    if (!genre.trimmed().isEmpty()) params.addQueryItem(QStringLiteral("genre"), genre.trimmed());
    url.setQuery(params);
    auto* reply = m_networkManager->get(requestFor(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() mutable {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
        } else {
            parseMixkitMusic(reply->readAll(), std::move(callback));
        }
        reply->deleteLater();
    });
}

void PremiumStockApi::searchSFX(const QString& category,
                                std::function<void(const QVector<StockSFX>&)> callback) {
    QUrl url(QStringLiteral("https://mixkit.co/api/sound-effects"));
    QUrlQuery params;
    if (!category.trimmed().isEmpty()) params.addQueryItem(QStringLiteral("category"), category.trimmed());
    url.setQuery(params);
    auto* reply = m_networkManager->get(requestFor(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() mutable {
        QVector<StockSFX> results;
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
        } else {
            QJsonParseError error{};
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                emit errorOccurred(error.errorString());
            } else {
                const QJsonArray items = doc.object().value(QStringLiteral("soundEffects")).toArray();
                results.reserve(items.size());
                for (const auto& value : items) {
                    const QJsonObject item = value.toObject();
                    StockSFX sfx;
                    sfx.id = item.value(QStringLiteral("id")).toVariant().toString();
                    sfx.title = item.value(QStringLiteral("title")).toString();
                    sfx.category = item.value(QStringLiteral("category")).toString();
                    sfx.duration = item.value(QStringLiteral("duration")).toString();
                    sfx.license = QStringLiteral("Mixkit Free License");
                    const QJsonObject files = item.value(QStringLiteral("audio_files")).toObject();
                    sfx.url = files.value(QStringLiteral("mp3")).toString();
                    sfx.previewUrl = files.value(QStringLiteral("preview")).toString();
                    if (!sfx.id.isEmpty()) results.append(sfx);
                }
            }
        }
        if (callback) callback(results);
        emit sfxReady(results);
        reply->deleteLater();
    });
}

void PremiumStockApi::getMotionGraphics(std::function<void(const QVector<MotionGraphic>&)> callback) {
    QUrl url(QStringLiteral("https://mixkit.co/api/free-video-assets/premium-after-effects-templates"));
    auto* reply = m_networkManager->get(requestFor(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() mutable {
        QVector<MotionGraphic> results;
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
        } else {
            QJsonParseError error{};
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                emit errorOccurred(error.errorString());
            } else {
                for (const auto& value : doc.object().value(QStringLiteral("assets")).toArray()) {
                    const QJsonObject item = value.toObject();
                    MotionGraphic mg;
                    mg.id = item.value(QStringLiteral("id")).toVariant().toString();
                    mg.title = item.value(QStringLiteral("title")).toString();
                    mg.previewUrl = item.value(QStringLiteral("preview_url")).toString();
                    mg.downloadUrl = item.value(QStringLiteral("download_url")).toString();
                    mg.type = item.value(QStringLiteral("type")).toString();
                    mg.license = QStringLiteral("Mixkit Free License");
                    if (!mg.id.isEmpty()) results.append(mg);
                }
            }
        }
        if (callback) callback(results);
        emit motionGraphicsReady(results);
        reply->deleteLater();
    });
}

void PremiumStockApi::searchCoverrVideos(const QString& query, int page,
                                         std::function<void(const QVector<StockVideo>&)> callback) {
    QUrl url(QStringLiteral("https://coverr.co/api/search"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("query"), query.trimmed());
    params.addQueryItem(QStringLiteral("page"), QString::number(std::max(1, page)));
    url.setQuery(params);
    auto* reply = m_networkManager->get(requestFor(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() mutable {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            if (callback) callback({});
        } else {
            parseCoverrVideos(reply->readAll(), std::move(callback));
        }
        reply->deleteLater();
    });
}

void PremiumStockApi::getTrendingVideos(std::function<void(const QVector<StockVideo>&)> callback) {
    searchVideos(QStringLiteral("trending"), 1, std::move(callback));
}

void PremiumStockApi::getVerticalVideos(std::function<void(const QVector<StockVideo>&)> callback) {
    searchVideos(QStringLiteral("vertical"), 1, std::move(callback));
}

void PremiumStockApi::searchPixabayVideos(const QString& query, const QString& apiKey, int page,
                                          std::function<void(const QVector<StockVideo>&)> callback) {
    if (apiKey.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("Pixabay API key is required"));
        if (callback) callback({});
        return;
    }
    QUrl url(QStringLiteral("https://pixabay.com/api/videos/"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), apiKey.trimmed());
    params.addQueryItem(QStringLiteral("q"), query.trimmed());
    params.addQueryItem(QStringLiteral("page"), QString::number(std::max(1, page)));
    params.addQueryItem(QStringLiteral("per_page"), QStringLiteral("20"));
    url.setQuery(params);
    auto* reply = m_networkManager->get(requestFor(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() mutable {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
        } else {
            parsePixabayVideos(reply->readAll(), std::move(callback));
        }
        reply->deleteLater();
    });
}

void PremiumStockApi::searchVidevo(const QString& query, const QString& apiKey,
                                   std::function<void(const QVector<StockVideo>&)> callback) {
    Q_UNUSED(query);
    Q_UNUSED(apiKey);
    unsupported(QStringLiteral("Videvo"), this, std::move(callback));
}

void PremiumStockApi::getAmbientMusic(const QString& activity,
                                      std::function<void(const QVector<StockMusic>&)> callback) {
    // No longer references an uncaptured callback parameter. The provider remains
    // informational until an authenticated, redistribution-safe API is configured.
    Q_UNUSED(activity);
    if (callback) callback({});
    emit musicReady({});
}

void PremiumStockApi::searchBensound(const QString& query, const QString& apiKey,
                                     std::function<void(const QVector<StockMusic>&)> callback) {
    Q_UNUSED(query);
    Q_UNUSED(apiKey);
    if (callback) callback({});
    emit musicReady({});
}

void PremiumStockApi::getMazwaiVideos(const QString& category,
                                      std::function<void(const QVector<StockVideo>&)> callback) {
    Q_UNUSED(category);
    unsupported(QStringLiteral("Mazwai"), this, std::move(callback));
}

void PremiumStockApi::getLifeOfVids(const QString& type,
                                    std::function<void(const QVector<StockVideo>&)> callback) {
    Q_UNUSED(type);
    unsupported(QStringLiteral("Life of Vids"), this, std::move(callback));
}

void PremiumStockApi::parseMixkitVideos(const QByteArray& data,
                                        std::function<void(const QVector<StockVideo>&)> callback) {
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    QVector<StockVideo> results;
    if (error.error == QJsonParseError::NoError) {
        const QJsonArray items = document.isArray() ? document.array()
            : document.object().value(QStringLiteral("videos")).toArray();
        results = parseVideoArray(items, QStringLiteral("Mixkit Free License"));
    } else {
        emit errorOccurred(error.errorString());
    }
    if (callback) callback(results);
    emit videosReady(results);
}

void PremiumStockApi::parseMixkitMusic(const QByteArray& data,
                                       std::function<void(const QVector<StockMusic>&)> callback) {
    QVector<StockMusic> results;
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    if (error.error == QJsonParseError::NoError) {
        const QJsonArray items = document.isArray() ? document.array()
            : document.object().value(QStringLiteral("music")).toArray();
        results.reserve(items.size());
        for (const auto& value : items) {
            const QJsonObject item = value.toObject();
            StockMusic music;
            music.id = item.value(QStringLiteral("id")).toVariant().toString();
            music.title = item.value(QStringLiteral("title")).toString();
            music.artist = item.value(QStringLiteral("artist")).toString();
            music.url = item.value(QStringLiteral("url")).toString();
            music.previewUrl = item.value(QStringLiteral("preview_url")).toString();
            music.genre = item.value(QStringLiteral("genre")).toString();
            music.license = QStringLiteral("Mixkit Free License");
            if (!music.id.isEmpty()) results.append(music);
        }
    } else {
        emit errorOccurred(error.errorString());
    }
    if (callback) callback(results);
    emit musicReady(results);
}

void PremiumStockApi::parseCoverrVideos(const QByteArray& data,
                                        std::function<void(const QVector<StockVideo>&)> callback) {
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    QVector<StockVideo> results;
    if (error.error == QJsonParseError::NoError) {
        const QJsonArray items = document.isArray() ? document.array()
            : document.object().value(QStringLiteral("results")).toArray();
        results = parseVideoArray(items, QStringLiteral("Coverr License"));
    } else {
        emit errorOccurred(error.errorString());
    }
    if (callback) callback(results);
    emit videosReady(results);
}

void PremiumStockApi::parsePixabayVideos(const QByteArray& data,
                                         std::function<void(const QVector<StockVideo>&)> callback) {
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    QVector<StockVideo> results;
    if (error.error == QJsonParseError::NoError) {
        results = parseVideoArray(document.object().value(QStringLiteral("hits")).toArray(), QStringLiteral("Pixabay Content License"));
        for (int i = 0; i < results.size(); ++i) {
            const QJsonObject hit = document.object().value(QStringLiteral("hits")).toArray().at(i).toObject();
            const QJsonObject videos = hit.value(QStringLiteral("videos")).toObject();
            results[i].url = videos.value(QStringLiteral("medium")).toObject().value(QStringLiteral("url")).toString();
            results[i].thumbnailUrl = hit.value(QStringLiteral("userImageURL")).toString();
        }
    } else {
        emit errorOccurred(error.errorString());
    }
    if (callback) callback(results);
    emit videosReady(results);
}

} // namespace ccos::api
