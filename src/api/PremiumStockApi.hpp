#pragma once
/**
 * @file PremiumStockApi.hpp
 * @brief External stock/media provider adapters.
 *
 * Providers have independent APIs, licenses, attribution rules and terms.
 * CCOS stores provider/license metadata but does not assume that any returned
 * asset is MIT, CC0, public domain or attribution-free.
 *
 * Current adapter families include Mixkit, Coverr, Pixabay, Videvo, Tuna,
 * Bensound, Mazwai and Life of Vids. Availability and API contracts must be
 * treated as external-provider capabilities and validated at runtime.
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
    QString type;
    QString license;
};

class PremiumStockApi : public QObject {
    Q_OBJECT

public:
    explicit PremiumStockApi(QObject* parent = nullptr);
    ~PremiumStockApi() override;

    void searchVideos(const QString& query, int page = 1,
                      std::function<void(const QVector<StockVideo>&)> callback);
    void searchMusic(const QString& genre, std::function<void(const QVector<StockMusic>&)> callback);
    void searchSFX(const QString& category, std::function<void(const QVector<StockSFX>&)> callback);
    void getMotionGraphics(std::function<void(const QVector<MotionGraphic>&)> callback);

    void searchCoverrVideos(const QString& query, int page = 1,
                            std::function<void(const QVector<StockVideo>&)> callback);
    void getTrendingVideos(std::function<void(const QVector<StockVideo>&)> callback);
    void getVerticalVideos(std::function<void(const QVector<StockVideo>&)> callback);

    void searchPixabayVideos(const QString& query, const QString& apiKey, int page = 1,
                             std::function<void(const QVector<StockVideo>&)> callback);

    void searchVidevo(const QString& query, const QString& apiKey,
                      std::function<void(const QVector<StockVideo>&)> callback);

    void getAmbientMusic(const QString& activity,
                         std::function<void(const QVector<StockMusic>&)> callback);

    void searchBensound(const QString& query, const QString& apiKey,
                        std::function<void(const QVector<StockMusic>&)> callback);

    void getMazwaiVideos(const QString& category,
                         std::function<void(const QVector<StockVideo>&)> callback);

    void getLifeOfVids(const QString& type,
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
