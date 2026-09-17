#include "SocialMediaApi.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrlQuery>
#include <QRegularExpression>

namespace ccos::api {

SocialMediaApi::SocialMediaApi(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

SocialMediaApi::~SocialMediaApi() = default;

void SocialMediaApi::setYouTubeApiKey(const QString& key) {
    m_youtubeApiKey = key;
}

void SocialMediaApi::setVimeoAccessToken(const QString& token) {
    m_vimeoAccessToken = token;
}

void SocialMediaApi::setTwitchClientId(const QString& clientId, const QString& clientSecret) {
    m_twitchClientId = clientId;
    m_twitchClientSecret = clientSecret;
    requestTwitchAccessToken();
}

void SocialMediaApi::setGiphyApiKey(const QString& key) {
    m_giphyApiKey = key;
}

void SocialMediaApi::setTenorApiKey(const QString& key) {
    m_tenorApiKey = key;
}

void SocialMediaApi::setRedditUserAgent(const QString& userAgent) {
    m_redditUserAgent = userAgent;
}

// ==================== YOUTUBE DATA API ====================
// Quota gratuita: 10,000 unidades/día
// Search: 100 unidades, Video Details: 1 unidad

void SocialMediaApi::searchYouTube(const QString& query, const QString& order,
                                   std::function<void(const QVector<SocialVideo>&)> callback)
{
    if (m_youtubeApiKey.isEmpty()) {
        emit errorOccurred("YouTube API key required. Get free key at console.cloud.google.com");
        return;
    }
    
    QUrl url("https://www.googleapis.com/youtube/v3/search");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("part", "snippet");
    queryBuilder.addQueryItem("q", query);
    queryBuilder.addQueryItem("type", "video");
    queryBuilder.addQueryItem("order", order);
    queryBuilder.addQueryItem("maxResults", "25");
    queryBuilder.addQueryItem("key", m_youtubeApiKey);
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<SocialVideo> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["items"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                QJsonObject snippet = item["snippet"].toObject();
                
                SocialVideo video;
                video.id = item["id"].toObject()["videoId"].toString();
                video.title = snippet["title"].toString();
                video.description = snippet["description"].toString();
                video.thumbnailUrl = snippet["thumbnails"].toObject()["high"].toObject()["url"].toString();
                video.author = snippet["channelTitle"].toString();
                video.publishedAt = snippet["publishedAt"].toString();
                video.platform = "youtube";
                video.embedUrl = "https://www.youtube.com/embed/" + video.id;
                
                results.append(video);
            }
            
            // Obtener detalles adicionales (views, likes, duration)
            if (!results.isEmpty()) {
                QStringList videoIds;
                for (const auto& v : results) {
                    videoIds.append(v.id);
                }
                
                QUrl detailsUrl("https://www.googleapis.com/youtube/v3/videos");
                QUrlQuery dq;
                dq.addQueryItem("part", "contentDetails,statistics");
                dq.addQueryItem("id", videoIds.join(","));
                dq.addQueryItem("key", m_youtubeApiKey);
                detailsUrl.setQuery(dq);
                
                auto* detailsReply = m_networkManager->get(QNetworkRequest(detailsUrl));
                connect(detailsReply, &QNetworkReply::finished, this, [detailsReply, &results, callback]() {
                    if (detailsReply->error() == QNetworkReply::NoError) {
                        QJsonDocument doc = QJsonDocument::fromJson(detailsReply->readAll());
                        QJsonObject root = doc.object();
                        QJsonArray items = root["items"].toArray();
                        
                        int i = 0;
                        for (const QJsonValue& val : items) {
                            if (i < results.size()) {
                                QJsonObject item = val.toObject();
                                QJsonObject stats = item["statistics"].toObject();
                                QJsonObject contentDetails = item["contentDetails"].toObject();
                                
                                results[i].views = stats["viewCount"].toString().toInt();
                                results[i].likes = stats["likeCount"].toString().toInt();
                                results[i].duration = contentDetails["duration"].toString();
                            }
                            i++;
                        }
                        callback(results);
                    }
                    detailsReply->deleteLater();
                });
            } else {
                callback(results);
            }
        }
        
        emit videosReady(results);
        reply->deleteLater();
    });
}

void SocialMediaApi::getYouTubeTrending(const QString& regionCode,
                                        std::function<void(const QVector<SocialVideo>&)> callback)
{
    if (m_youtubeApiKey.isEmpty()) {
        emit errorOccurred("YouTube API key required");
        return;
    }
    
    QUrl url("https://www.googleapis.com/youtube/v3/videos");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("part", "snippet,statistics,contentDetails");
    queryBuilder.addQueryItem("chart", "mostPopular");
    queryBuilder.addQueryItem("regionCode", regionCode);
    queryBuilder.addQueryItem("maxResults", "25");
    queryBuilder.addQueryItem("key", m_youtubeApiKey);
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<SocialVideo> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["items"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                QJsonObject snippet = item["snippet"].toObject();
                QJsonObject stats = item["statistics"].toObject();
                QJsonObject contentDetails = item["contentDetails"].toObject();
                
                SocialVideo video;
                video.id = item["id"].toString();
                video.title = snippet["title"].toString();
                video.description = snippet["description"].toString();
                video.thumbnailUrl = snippet["thumbnails"].toObject()["high"].toObject()["url"].toString();
                video.author = snippet["channelTitle"].toString();
                video.publishedAt = snippet["publishedAt"].toString();
                video.views = stats["viewCount"].toString().toInt();
                video.likes = stats["likeCount"].toString().toInt();
                video.duration = contentDetails["duration"].toString();
                video.platform = "youtube";
                video.embedUrl = "https://www.youtube.com/embed/" + video.id;
                
                results.append(video);
            }
        }
        
        callback(results);
        emit videosReady(results);
        reply->deleteLater();
    });
}

// ==================== VIMEO API ====================
// Tier gratuito: 500 requests/día

void SocialMediaApi::searchVimeo(const QString& query, const QString& sort,
                                 std::function<void(const QVector<SocialVideo>&)> callback)
{
    QUrl url("https://api.vimeo.com/videos");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("query", query);
    queryBuilder.addQueryItem("sort", sort);
    queryBuilder.addQueryItem("per_page", "25");
    url.setQuery(queryBuilder);
    
    QNetworkRequest request(url);
    if (!m_vimeoAccessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_vimeoAccessToken.toUtf8());
    }
    
    auto* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<SocialVideo> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["data"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                
                SocialVideo video;
                video.id = item["uri"].toString().split("/").last();
                video.title = item["name"].toString();
                video.description = item["description"].toString();
                video.thumbnailUrl = item["pictures"].toObject()["sizes"].toArray()[0].toObject()["link"].toString();
                video.author = item["user"].toObject()["name"].toString();
                video.duration = QString::number(item["duration"].toInt()) + "s";
                video.views = item["stats"].toObject()["plays"].toInt();
                video.likes = item["metadata"].toObject()["connections"].toObject()["likes"].toObject()["total"].toInt();
                video.publishedAt = item["created_time"].toString();
                video.platform = "vimeo";
                video.embedUrl = "https://player.vimeo.com/video/" + video.id;
                
                results.append(video);
            }
        }
        
        callback(results);
        emit videosReady(results);
        reply->deleteLater();
    });
}

void SocialMediaApi::getVimeoStaffPicks(std::function<void(const QVector<SocialVideo>&)> callback)
{
    QUrl url("https://api.vimeo.com/channels/staffpicks/videos?per_page=25");
    
    QNetworkRequest request(url);
    if (!m_vimeoAccessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_vimeoAccessToken.toUtf8());
    }
    
    auto* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<SocialVideo> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["data"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                
                SocialVideo video;
                video.id = item["uri"].toString().split("/").last();
                video.title = item["name"].toString();
                video.description = item["description"].toString();
                video.thumbnailUrl = item["pictures"].toObject()["sizes"].toArray()[0].toObject()["link"].toString();
                video.author = item["user"].toObject()["name"].toString();
                video.duration = QString::number(item["duration"].toInt()) + "s";
                video.platform = "vimeo";
                video.embedUrl = "https://player.vimeo.com/video/" + video.id;
                
                results.append(video);
            }
        }
        
        callback(results);
        emit videosReady(results);
        reply->deleteLater();
    });
}

// ==================== GIPHY API ====================
// Gratis con attribution, rate limit: 1000 requests/hora

void SocialMediaApi::searchGifs(const QString& query, const QString& rating,
                                std::function<void(const QVector<GifAsset>&)> callback)
{
    if (m_giphyApiKey.isEmpty()) {
        emit errorOccurred("Giphy API key required. Get free key at developers.giphy.com");
        return;
    }
    
    QUrl url("https://api.giphy.com/v1/gifs/search");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("api_key", m_giphyApiKey);
    queryBuilder.addQueryItem("q", query);
    queryBuilder.addQueryItem("limit", "25");
    queryBuilder.addQueryItem("rating", rating);
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<GifAsset> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["data"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                QJsonObject images = item["images"].toObject();
                QJsonObject original = images["original"].toObject();
                QJsonObject preview = images["downsized_small"].toObject();
                
                GifAsset gif;
                gif.id = item["id"].toString();
                gif.title = item["title"].toString();
                gif.url = original["url"].toString();
                gif.previewUrl = preview["url"].toString();
                gif.downloadUrl = original["download_url"].toString();
                gif.width = original["width"].toInt();
                gif.height = original["height"].toInt();
                gif.platform = "giphy";
                
                results.append(gif);
            }
        }
        
        callback(results);
        emit gifsReady(results);
        reply->deleteLater();
    });
}

void SocialMediaApi::getTrendingGifs(int limit,
                                     std::function<void(const QVector<GifAsset>&)> callback)
{
    if (m_giphyApiKey.isEmpty()) {
        emit errorOccurred("Giphy API key required");
        return;
    }
    
    QUrl url("https://api.giphy.com/v1/gifs/trending");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("api_key", m_giphyApiKey);
    queryBuilder.addQueryItem("limit", QString::number(limit));
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<GifAsset> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["data"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                QJsonObject images = item["images"].toObject();
                QJsonObject original = images["original"].toObject();
                
                GifAsset gif;
                gif.id = item["id"].toString();
                gif.title = item["title"].toString();
                gif.url = original["url"].toString();
                gif.previewUrl = images["downsized"].toObject()["url"].toString();
                gif.downloadUrl = original["download_url"].toString();
                gif.width = original["width"].toInt();
                gif.height = original["height"].toInt();
                gif.platform = "giphy";
                
                results.append(gif);
            }
        }
        
        callback(results);
        emit gifsReady(results);
        reply->deleteLater();
    });
}

// ==================== TENOR API ====================
// Gratis con attribution, rate limit: 1000 requests/día

void SocialMediaApi::searchTenor(const QString& query, const QString& mediaFilter,
                                 std::function<void(const QVector<GifAsset>&)> callback)
{
    if (m_tenorApiKey.isEmpty()) {
        emit errorOccurred("Tenor API key required. Get free key at tenor.com/gifapi");
        return;
    }
    
    QUrl url("https://tenor.googleapis.com/v2/search");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("key", m_tenorApiKey);
    queryBuilder.addQueryItem("q", query);
    queryBuilder.addQueryItem("media_filter", mediaFilter);
    queryBuilder.addQueryItem("limit", "25");
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<GifAsset> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["results"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                QJsonObject media = item["media_formats"].toObject();
                QJsonObject gif = media["gif"].toObject();
                
                GifAsset gifAsset;
                gifAsset.id = item["id"].toString();
                gifAsset.title = item["content_description"].toString();
                gifAsset.url = gif["url"].toString();
                gifAsset.previewUrl = media["tinygif"].toObject()["url"].toString();
                gifAsset.downloadUrl = gif["url"].toString();
                gifAsset.width = gif["dims"].toArray()[0].toInt();
                gifAsset.height = gif["dims"].toArray()[1].toInt();
                gifAsset.platform = "tenor";
                
                results.append(gifAsset);
            }
        }
        
        callback(results);
        emit gifsReady(results);
        reply->deleteLater();
    });
}

void SocialMediaApi::getTenorCategories(std::function<void(const QStringList&)> callback)
{
    if (m_tenorApiKey.isEmpty()) {
        emit errorOccurred("Tenor API key required");
        return;
    }
    
    QUrl url("https://tenor.googleapis.com/v2/categories");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("key", m_tenorApiKey);
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QStringList categories;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["tags"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                categories.append(item["search_term"].toString());
            }
        }
        
        callback(categories);
        reply->deleteLater();
    });
}

// ==================== REDDIT API ====================
// Lectura pública sin autenticación (con User-Agent)

void SocialMediaApi::getHotPosts(const QString& subreddit, int limit,
                                 std::function<void(const QVector<SocialPost>&)> callback)
{
    if (m_redditUserAgent.isEmpty()) {
        emit errorOccurred("Reddit requires User-Agent header. Set with setRedditUserAgent()");
        return;
    }
    
    QUrl url(QString("https://www.reddit.com/r/%1/hot.json?limit=%2")
             .arg(subreddit).arg(limit));
    
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_redditUserAgent.toUtf8());
    
    auto* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<SocialPost> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray children = root["data"].toObject()["children"].toArray();
            
            for (const QJsonValue& val : children) {
                QJsonObject child = val.toObject()["data"].toObject();
                
                SocialPost post;
                post.id = child["id"].toString();
                post.content = child["title"].toString();
                post.author = child["author"].toString();
                post.likes = child["ups"].toInt();
                post.comments = child["num_comments"].toInt();
                post.createdAt = QDateTime::fromSecsSinceEpoch(child["created_utc"].toInt()).toString();
                post.platform = "reddit";
                
                if (!child["thumbnail"].toString().isEmpty() && 
                    child["thumbnail"].toString() != "self" &&
                    child["thumbnail"].toString() != "default") {
                    post.imageUrl = child["thumbnail"].toString();
                }
                
                if (child["is_video"].toBool() && !child["media"].isNull()) {
                    post.videoUrl = child["media"].toObject()["reddit_video"].toObject()["fallback_url"].toString();
                }
                
                results.append(post);
            }
        }
        
        callback(results);
        emit postReady(results);
        reply->deleteLater();
    });
}

// ==================== TWITCH API ====================
// Requiere OAuth2, Client ID y Client Secret

void SocialMediaApi::requestTwitchAccessToken()
{
    if (m_twitchClientId.isEmpty() || m_twitchClientSecret.isEmpty()) {
        return;
    }
    
    QUrl url("https://id.twitch.tv/oauth2/token");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("client_id", m_twitchClientId);
    queryBuilder.addQueryItem("client_secret", m_twitchClientSecret);
    queryBuilder.addQueryItem("grant_type", "client_credentials");
    url.setQuery(queryBuilder);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    auto* reply = m_networkManager->post(request, QByteArray());
    
    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                m_twitchAccessToken = doc.object()["access_token"].toString();
            }
        }
        reply->deleteLater();
    });
}

void SocialMediaApi::getTopStreams(int limit,
                                   std::function<void(const QVector<StreamInfo>&)> callback)
{
    if (m_twitchAccessToken.isEmpty()) {
        emit errorOccurred("Twitch access token required. Call setTwitchClientId() first.");
        return;
    }
    
    QUrl url("https://api.twitch.tv/helix/streams");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("first", QString::number(limit));
    url.setQuery(queryBuilder);
    
    QNetworkRequest request(url);
    request.setRawHeader("Client-Id", m_twitchClientId.toUtf8());
    request.setRawHeader("Authorization", ("Bearer " + m_twitchAccessToken).toUtf8());
    
    auto* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<StreamInfo> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["data"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                
                StreamInfo stream;
                stream.channelId = item["user_id"].toString();
                stream.channelName = item["user_name"].toString();
                stream.title = item["title"].toString();
                stream.game = item["game_name"].toString();
                stream.thumbnailUrl = item["thumbnail_url"].toString()
                    .replace("{width}", "640").replace("{height}", "360");
                stream.viewers = item["viewer_count"].toInt();
                stream.isLive = true;
                stream.startedAt = item["started_at"].toString();
                stream.language = item["language"].toString();
                
                results.append(stream);
            }
        }
        
        callback(results);
        emit streamsReady(results);
        reply->deleteLater();
    });
}

// ==================== UTILIDADES ====================

QString SocialMediaApi::extractVideoId(const QString& url, const QString& platform)
{
    if (platform == "youtube") {
        QRegularExpression re(R"(youtu(?:\.be|be\.com)/(?:.*v(?:/|=)|(?:shorts/))([a-zA-Z0-9-_]+))");
        QRegularExpressionMatch match = re.match(url);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    } else if (platform == "vimeo") {
        QRegularExpression re(R"(vimeo\.com/(?:channels/(?:\w+/)?|groups/(?:\w+)/video/)?(\d+))");
        QRegularExpressionMatch match = re.match(url);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    
    return QString();
}

QString SocialMediaApi::generateEmbedCode(const SocialVideo& video, int width, int height)
{
    if (video.platform == "youtube") {
        return QString("<iframe width=\"%1\" height=\"%2\" src=\"%3\" "
                      "frameborder=\"0\" allowfullscreen></iframe>")
            .arg(width).arg(height).arg(video.embedUrl);
    } else if (video.platform == "vimeo") {
        return QString("<iframe src=\"%1\" width=\"%2\" height=\"%3\" "
                      "frameborder=\"0\" allowfullscreen></iframe>")
            .arg(video.embedUrl).arg(width).arg(height);
    }
    
    return QString();
}

} // namespace ccos::api
