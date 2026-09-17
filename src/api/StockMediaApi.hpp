#pragma once
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace ccos::api {

struct StockMediaResult {
    QString id;
    QString type; // "video" or "image"
    QString url;
    QString thumbnailUrl;
    int width = 0;
    int height = 0;
    double durationSec = 0.0;
    QString author;
    QString license;
    QString downloadUrl;
};

class StockMediaApi {
public:
    explicit StockMediaApi(QString apiKey = QString());
    
    [[nodiscard]] QString provider() const { return provider_; }
    void setProvider(const QString& provider);
    
    QVector<StockMediaResult> searchVideos(const QString& query, int page = 1, int perPage = 15);
    QVector<StockMediaResult> searchImages(const QString& query, int page = 1, int perPage = 15);
    QString getDownloadUrl(const StockMediaResult& result);
    
    static QStringList supportedProviders();
    
private:
    QString apiKey_;
    QString provider_ = QStringLiteral("pexels");
    
    QVector<StockMediaResult> searchPexels(const QString& query, const QString& type, int page, int perPage);
    QVector<StockMediaResult> searchPixabay(const QString& query, const QString& type, int page, int perPage);
};

} // namespace ccos::api
