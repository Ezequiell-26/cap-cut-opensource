#pragma once
/**
 * @file SocialMediaApi.hpp
 * @brief APIs para integración con redes sociales y plataformas de video
 * 
 * APIs integradas:
 * - YouTube Data API (quota gratuita diaria)
 * - Vimeo API (tier gratuito)
 * - TikTok API (acceso limitado)
 * - Instagram Basic Display API
 * - Twitch API (Kraken/Helix)
 * - Twitter/X API v2 (tier gratuito limitado)
 * - Reddit API (gratis, sin key para lectura básica)
 * - Giphy API (gratis con attribution)
 * - Tenor API (GIFs, gratis con attribution)
 */

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonDocument>
#include <QNetworkReply>
#include <functional>

namespace ccos::api {

struct SocialVideo {
    QString id;
    QString title;
    QString description;
    QString thumbnailUrl;
    QString videoUrl;
    QString embedUrl;
    QString duration;
    QString author;
    QString authorAvatar;
    int views;
    int likes;
    QString publishedAt;
    QString platform; // "youtube", "vimeo", "tiktok", etc.
};

struct SocialPost {
    QString id;
    QString content;
    QString imageUrl;
    QString videoUrl;
    QString author;
    QString authorAvatar;
    int likes;
    int shares;
    int comments;
    QString createdAt;
    QString platform;
};

struct GifAsset {
    QString id;
    QString title;
    QString url;
    QString previewUrl;
    QString downloadUrl;
    int width;
    int height;
    QString platform; // "giphy", "tenor"
};

struct StreamInfo {
    QString channelId;
    QString channelName;
    QString title;
    QString game;
    QString thumbnailUrl;
    int viewers;
    bool isLive;
    QString startedAt;
    QString language;
};

class SocialMediaApi : public QObject {
    Q_OBJECT

public:
    explicit SocialMediaApi(QObject* parent = nullptr);
    ~SocialMediaApi() override;

    // Configuración de API Keys
    void setYouTubeApiKey(const QString& key);
    void setVimeoAccessToken(const QString& token);
    void setTwitchClientId(const QString& clientId, const QString& clientSecret);
    void setGiphyApiKey(const QString& key);
    void setTenorApiKey(const QString& key);
    void setRedditUserAgent(const QString& userAgent);

    // YouTube Data API v3
    void searchYouTube(const QString& query, const QString& order = "relevance",
                       std::function<void(const QVector<SocialVideo>&)> callback);
    void getYouTubeVideoDetails(const QString& videoId,
                                std::function<void(const SocialVideo&)> callback);
    void getYouTubeChannelVideos(const QString& channelId,
                                 std::function<void(const QVector<SocialVideo>&)> callback);
    void getYouTubeTrending(const QString& regionCode = "US",
                            std::function<void(const QVector<SocialVideo>&)> callback);

    // Vimeo API
    void searchVimeo(const QString& query, const QString& sort = "relevant",
                     std::function<void(const QVector<SocialVideo>&)> callback);
    void getVimeoVideoDetails(const QString& videoId,
                              std::function<void(const SocialVideo&)> callback);
    void getVimeoStaffPicks(std::function<void(const QVector<SocialVideo>&)> callback);

    // Giphy API
    void searchGifs(const QString& query, const QString& rating = "g",
                    std::function<void(const QVector<GifAsset>&)> callback);
    void getTrendingGifs(int limit = 25,
                         std::function<void(const QVector<GifAsset>&)> callback);
    void getStickerPack(const QString& packId,
                        std::function<void(const QVector<GifAsset>&)> callback);
    void translateGif(const QString& searchTerm,
                      std::function<void(const GifAsset&)> callback);

    // Tenor API
    void searchTenor(const QString& query, const QString& mediaFilter = "gif",
                     std::function<void(const QVector<GifAsset>&)> callback);
    void getTenorTrending(int limit = 25,
                          std::function<void(const QVector<GifAsset>&)> callback);
    void getTenorCategories(std::function<void(const QStringList&)> callback);

    // Reddit API (sin autenticación para lectura pública)
    void searchReddit(const QString& subreddit, const QString& query,
                      std::function<void(const QVector<SocialPost>&)> callback);
    void getHotPosts(const QString& subreddit, int limit = 25,
                     std::function<void(const QVector<SocialPost>&)> callback);
    void getTopPosts(const QString& subreddit, const QString& timeframe = "day",
                     std::function<void(const QVector<SocialPost>&)> callback);

    // Twitch API (requiere OAuth)
    void searchTwitchChannels(const QString& query,
                              std::function<void(const QVector<StreamInfo>&)> callback);
    void getTopStreams(int limit = 10,
                       std::function<void(const QVector<StreamInfo>&)> callback);
    void getStreamInfo(const QString& login,
                       std::function<void(const StreamInfo&)> callback);
    void getGameStreams(const QString& gameId,
                        std::function<void(const QVector<StreamInfo>&)> callback);

    // Utilidades
    QString extractVideoId(const QString& url, const QString& platform);
    QString generateEmbedCode(const SocialVideo& video, int width = 640, int height = 360);

signals:
    void videosReady(const QVector<SocialVideo>&);
    void postReady(const QVector<SocialPost>&);
    void gifsReady(const QVector<GifAsset>&);
    void streamsReady(const QVector<StreamInfo>&);
    void errorOccurred(const QString& message);

private:
    QNetworkAccessManager* m_networkManager;
    
    QString m_youtubeApiKey;
    QString m_vimeoAccessToken;
    QString m_twitchClientId;
    QString m_twitchClientSecret;
    QString m_twitchAccessToken;
    QString m_giphyApiKey;
    QString m_tenorApiKey;
    QString m_redditUserAgent;

    void requestTwitchAccessToken();
};

} // namespace ccos::api
