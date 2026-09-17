#pragma once
/**
 * @file PremiumStockApi.hpp
 * @brief APIs de stock premium gratuito (Mixkit, Coverr, Tuna, Bensound)
 * 
 * APIs integradas con licencias MIT/CC0 para uso comercial sin atribución:
 * - Mixkit: Videos, música, SFX, plantillas After Effects
 * - Coverr: Videos de fondo 4K
 * - Tuna: Música ambient por categoría
 * - Bensound: Música royalty-free
 * - Pixabay Video: Videos HD/4K
 * - Videvo: Clips y motion graphics
 */

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonDocument>
#include <QNetworkReply>
#include <functional>

namespace ccos::api {

struct StockVideo {
    QString id;
    QString title;
    QString url;
    QString thumbnailUrl;
    QString duration;
    QString resolution;
    QString license;
    QString author;
};

struct StockMusic {
    QString id;
    QString title;
    QString artist;
    QString url;
    QString previewUrl;
    QString duration;
    QString genre;
    QString mood;
    QString license;
};

struct StockSFX {
    QString id;
    QString title;
    QString url;
    QString previewUrl;
    QString category;
    QString duration;
    QString license;
};

struct MotionGraphic {
    QString id;
    QString title;
    QString previewUrl;
    QString downloadUrl;
    QString type; // "after-effects", "premiere", "davinci"
    QString license;
};

class PremiumStockApi : public QObject {
    Q_OBJECT

public:
    explicit PremiumStockApi(QObject* parent = nullptr);
    ~PremiumStockApi() override;

    // Mixkit API (gratis, sin key requerida)
    void searchVideos(const QString& query, int page = 1,
                      std::function<void(const QVector<StockVideo>&)> callback);
    void searchMusic(const QString& genre, std::function<void(const QVector<StockMusic>&)> callback);
    void searchSFX(const QString& category, std::function<void(const QVector<StockSFX>&)> callback);
    void getMotionGraphics(std::function<void(const QVector<MotionGraphic>&)> callback);

    // Coverr API (videos de fondo)
    void searchCoverrVideos(const QString& query, int page = 1,
                            std::function<void(const QVector<StockVideo>&)> callback);
    void getTrendingVideos(std::function<void(const QVector<StockVideo>&)> callback);
    void getVerticalVideos(std::function<void(const QVector<StockVideo>&)> callback);

    // Pixabay Video API
    void searchPixabayVideos(const QString& query, const QString& apiKey, int page = 1,
                             std::function<void(const QVector<StockVideo>&)> callback);

    // Videvo API (requiere registro gratis)
    void searchVidevo(const QString& query, const QString& apiKey,
                      std::function<void(const QVector<StockVideo>&)> callback);

    // Tuna API (música ambient de Spotify)
    void getAmbientMusic(const QString& activity,
                         std::function<void(const QVector<StockMusic>&)> callback);

    // Bensound API (música royalty-free)
    void searchBensound(const QString& query, const QString& apiKey,
                        std::function<void(const QVector<StockMusic>&)> callback);

    // Mazwai (videos cinematográficos)
    void getMazwaiVideos(const QString& category,
                         std::function<void(const QVector<StockVideo>&)> callback);

    // Life of Vids (videos y loops gratis)
    void getLifeOfVids(const QString& type, // "video" o "loop"
                       std::function<void(const QVector<StockVideo>&)> callback);

signals:
    void videosReady(const QVector<StockVideo>&);
    void musicReady(const QVector<StockMusic>&);
    void sfxReady(const QVector<StockSFX>&);
    void motionGraphicsReady(const QVector<MotionGraphic>&);
    void errorOccurred(const QString& message);

private:
    QNetworkAccessManager* m_networkManager;
    
    void parseMixkitVideos(const QByteArray& data,
                           std::function<void(const QVector<StockVideo>&)> callback);
    void parseMixkitMusic(const QByteArray& data,
                          std::function<void(const QVector<StockMusic>&)> callback);
    void parseCoverrVideos(const QByteArray& data,
                           std::function<void(const QVector<StockVideo>&)> callback);
    void parsePixabayVideos(const QByteArray& data,
                            std::function<void(const QVector<StockVideo>&)> callback);
};

} // namespace ccos::api
