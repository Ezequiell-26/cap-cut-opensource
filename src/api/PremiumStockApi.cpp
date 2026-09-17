#include "PremiumStockApi.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrlQuery>

namespace ccos::api {

PremiumStockApi::PremiumStockApi(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

PremiumStockApi::~PremiumStockApi() = default;

// ==================== MIXKIT API ====================
// Mixkit no requiere API key y es completamente gratis
// Licencia: Mixkit License (uso comercial permitido)

void PremiumStockApi::searchVideos(const QString& query, int page,
                                   std::function<void(const QVector<StockVideo>&)> callback)
{
    QUrl url("https://mixkit.co/api/videos");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("search", query);
    queryBuilder.addQueryItem("page", QString::number(page));
    url.setQuery(queryBuilder);

    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        parseMixkitVideos(reply->readAll(), callback);
        reply->deleteLater();
    });
}

void PremiumStockApi::searchMusic(const QString& genre,
                                  std::function<void(const QVector<StockMusic>&)> callback)
{
    QUrl url("https://mixkit.co/api/music");
    QUrlQuery queryBuilder;
    if (!genre.isEmpty()) {
        queryBuilder.addQueryItem("genre", genre);
    }
    url.setQuery(queryBuilder);

    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        parseMixkitMusic(reply->readAll(), callback);
        reply->deleteLater();
    });
}

void PremiumStockApi::searchSFX(const QString& category,
                                std::function<void(const QVector<StockSFX>&)> callback)
{
    QUrl url("https://mixkit.co/api/sound-effects");
    QUrlQuery queryBuilder;
    if (!category.isEmpty()) {
        queryBuilder.addQueryItem("category", category);
    }
    url.setQuery(queryBuilder);

    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        // Parsear SFX de Mixkit
        QVector<StockSFX> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["soundEffects"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                StockSFX sfx;
                sfx.id = item["id"].toString();
                sfx.title = item["title"].toString();
                sfx.category = item["category"].toString();
                sfx.duration = item["duration"].toString();
                sfx.license = "Mixkit Free License";
                
                QJsonObject audioFiles = item["audio_files"].toObject();
                sfx.url = audioFiles["mp3"].toString();
                sfx.previewUrl = audioFiles["preview"].toString();
                
                results.append(sfx);
            }
        }
        
        callback(results);
        emit sfxReady(results);
    });
}

void PremiumStockApi::getMotionGraphics(std::function<void(const QVector<MotionGraphic>&)> callback)
{
    QUrl url("https://mixkit.co/api/free-video-assets/premium-after-effects-templates");
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<MotionGraphic> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["assets"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                MotionGraphic mg;
                mg.id = item["id"].toString();
                mg.title = item["title"].toString();
                mg.previewUrl = item["preview_url"].toString();
                mg.downloadUrl = item["download_url"].toString();
                mg.type = item["type"].toString(); // after-effects, premiere, etc.
                mg.license = "Mixkit Free License";
                
                results.append(mg);
            }
        }
        
        callback(results);
        emit motionGraphicsReady(results);
    });
}

// ==================== COVERR API ====================
// Videos de fondo 4K gratuitos
// Licencia: CC0 / Coverr License

void PremiumStockApi::searchCoverrVideos(const QString& query, int page,
                                         std::function<void(const QVector<StockVideo>&)> callback)
{
    QUrl url(QString("https://coverr.co/api/videos?query=%1&page=%2")
             .arg(query).arg(page));
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        parseCoverrVideos(reply->readAll(), callback);
        reply->deleteLater();
    });
}

void PremiumStockApi::getTrendingVideos(std::function<void(const QVector<StockVideo>&)> callback)
{
    QUrl url("https://coverr.co/api/videos/trending");
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        parseCoverrVideos(reply->readAll(), callback);
        reply->deleteLater();
    });
}

void PremiumStockApi::getVerticalVideos(std::function<void(const QVector<StockVideo>&)> callback)
{
    QUrl url("https://coverr.co/api/videos/vertical");
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        parseCoverrVideos(reply->readAll(), callback);
        reply->deleteLater();
    });
}

// ==================== PIXABAY VIDEO API ====================
// Requiere API key gratuita
// Licencia: Pixabay Content License (uso comercial permitido)

void PremiumStockApi::searchPixabayVideos(const QString& query, const QString& apiKey,
                                          int page,
                                          std::function<void(const QVector<StockVideo>&)> callback)
{
    if (apiKey.isEmpty()) {
        emit errorOccurred("Pixabay API key required. Get free key at pixabay.com/api/docs/");
        return;
    }
    
    QUrl url("https://pixabay.com/api/videos/");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("key", apiKey);
    queryBuilder.addQueryItem("q", query);
    queryBuilder.addQueryItem("per_page", "50");
    queryBuilder.addQueryItem("page", QString::number(page));
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        parsePixabayVideos(reply->readAll(), callback);
        reply->deleteLater();
    });
}

// ==================== TUNA API (Spotify Ambient) ====================
// Música ambient categorizada por actividad
// Licencia: Spotify API - uso personal

void PremiumStockApi::getAmbientMusic(const QString& activity,
                                      std::function<void(const QVector<StockMusic>&)> callback)
{
    // Tuna usa la API de Spotify para playlists curadas
    // Actividades: studying, working, exercising, relaxing, sleeping, etc.
    QUrl url(QString("https://tuna.api.spotifina.com/api/v1/%1/playlists").arg(activity));
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<StockMusic> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isArray()) {
            QJsonArray items = doc.array();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                StockMusic music;
                music.id = item["id"].toString();
                music.title = item["name"].toString();
                music.artist = item["owner"].toObject()["display_name"].toString();
                music.genre = activity;
                music.mood = activity;
                music.license = "Spotify - Personal Use Only";
                
                results.append(music);
            }
        }
        
        callback(results);
        emit musicReady(results);
    });
}

// ==================== MAZWAI ====================
// Videos cinematográficos de alta calidad
// Licencia: Creative Commons Attribution 3.0

void PremiumStockApi::getMazwaiVideos(const QString& category,
                                      std::function<void(const QVector<StockVideo>&)> callback)
{
    // Mazwai no tiene API oficial, scraping simulado
    // Categorías: slow-motion, aerial, timelapse, nature, urban, people
    QUrl url("https://mazwai.com/api/videos");
    QUrlQuery queryBuilder;
    if (!category.isEmpty()) {
        queryBuilder.addQueryItem("category", category);
    }
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<StockVideo> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isArray()) {
            QJsonArray items = doc.array();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                StockVideo video;
                video.id = item["id"].toString();
                video.title = item["title"].toString();
                video.url = item["videoUrl"].toString();
                video.thumbnailUrl = item["thumbnailUrl"].toString();
                video.duration = item["duration"].toString();
                video.resolution = item["resolution"].toString();
                video.license = "CC BY 3.0";
                video.author = item["author"].toString();
                
                results.append(video);
            }
        }
        
        callback(results);
        emit videosReady(results);
    });
}

// ==================== LIFE OF VIDS ====================
// Videos y loops gratuitos de la agencia LoV
// Licencia: Life of Vids License (uso comercial permitido)

void PremiumStockApi::getLifeOfVids(const QString& type,
                                    std::function<void(const QVector<StockVideo>&)> callback)
{
    // type: "video" o "loop"
    QUrl url(type == "loop" 
             ? "https://www.lifeofvids.com/api/loops/"
             : "https://www.lifeofvids.com/api/videos/");
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<StockVideo> results;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["clips"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                StockVideo video;
                video.id = item["ID"].toString();
                video.title = item["Title"].toString();
                video.url = item["URL"].toString();
                video.thumbnailUrl = item["Image"].toString();
                video.duration = item["Length"].toString();
                video.license = "Life of Vids License";
                
                results.append(video);
            }
        }
        
        callback(results);
        emit videosReady(results);
    });
}

// ==================== PARSERS ====================

void PremiumStockApi::parseMixkitVideos(const QByteArray& data,
                                        std::function<void(const QVector<StockVideo>&)> callback)
{
    QVector<StockVideo> results;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray items = root["videos"].toArray();
        
        for (const QJsonValue& val : items) {
            QJsonObject item = val.toObject();
            StockVideo video;
            video.id = item["id"].toString();
            video.title = item["title"].toString();
            video.url = item["video_files"].toArray()[0].toObject()["file"].toString();
            video.thumbnailUrl = item["image"].toString();
            video.duration = item["duration"].toString();
            video.resolution = item["width"].toString() + "x" + item["height"].toString();
            video.license = "Mixkit Free License";
            video.author = item["contributor"].toString();
            
            results.append(video);
        }
    }
    
    callback(results);
    emit videosReady(results);
}

void PremiumStockApi::parseMixkitMusic(const QByteArray& data,
                                       std::function<void(const QVector<StockMusic>&)> callback)
{
    QVector<StockMusic> results;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray items = root["music"].toArray();
        
        for (const QJsonValue& val : items) {
            QJsonObject item = val.toObject();
            StockMusic music;
            music.id = item["id"].toString();
            music.title = item["title"].toString();
            music.artist = item["artist"].toString();
            music.genre = item["genre"].toString();
            music.mood = item["mood"].toString();
            music.duration = item["duration"].toString();
            music.license = "Mixkit Free License";
            
            QJsonObject audioFiles = item["audio_files"].toObject();
            music.url = audioFiles["mp3"].toString();
            music.previewUrl = audioFiles["preview"].toString();
            
            results.append(music);
        }
    }
    
    callback(results);
    emit musicReady(results);
}

void PremiumStockApi::parseCoverrVideos(const QByteArray& data,
                                        std::function<void(const QVector<StockVideo>&)> callback)
{
    QVector<StockVideo> results;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray items = root["videos"].toArray();
        
        for (const QJsonValue& val : items) {
            QJsonObject item = val.toObject();
            StockVideo video;
            video.id = item["id"].toString();
            video.title = item["description"].toString();
            video.url = item["videoFiles"].toArray()[0].toObject()["link"].toString();
            video.thumbnailUrl = item["imageUrl"].toString();
            video.duration = item["length"].toString();
            video.resolution = item["width"].toString() + "x" + item["height"].toString();
            video.license = "Coverr License";
            video.author = item["user"].toObject()["name"].toString();
            
            results.append(video);
        }
    }
    
    callback(results);
    emit videosReady(results);
}

void PremiumStockApi::parsePixabayVideos(const QByteArray& data,
                                         std::function<void(const QVector<StockVideo>&)> callback)
{
    QVector<StockVideo> results;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray items = root["hits"].toArray();
        
        for (const QJsonValue& val : items) {
            QJsonObject item = val.toObject();
            StockVideo video;
            video.id = item["id"].toString();
            video.title = item["tags"].toString();
            video.url = item["videos"].toObject()["hd"].toObject()["url"].toString();
            video.thumbnailUrl = item["picture"].toString();
            video.duration = item["duration"].toString();
            video.resolution = item["width"].toString() + "x" + item["height"].toString();
            video.license = "Pixabay Content License";
            video.author = item["user"].toString();
            
            results.append(video);
        }
    }
    
    callback(results);
    emit videosReady(results);
}

} // namespace ccos::api
