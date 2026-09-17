#pragma once
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <QColor>

namespace ccos::api {

struct FontInfo {
    QString family;
    QString variant;
    QString category; // serif, sans-serif, display, handwriting, monospace
    QString license;
    QString downloadUrl;
    QStringList subsets; // latin, cyrillic, greek, etc.
};

struct IconData {
    QString name;
    QString id;
    QString svgPath;
    QString unicode;
    QStringList categories;
};

class GoogleFontsApi {
public:
    explicit GoogleFontsApi(QString apiKey = QString());

    [[nodiscard]] QString provider() const { return provider_; }
    void setProvider(const QString& provider);

    // Google Fonts API
    QVector<FontInfo> listFonts(const QString& subset = "latin", int maxResults = 100);
    FontInfo getFontDetails(const QString& fontFamily);
    QString getFontDownloadUrl(const QString& fontFamily, const QString& variant = "regular");
    QVector<FontInfo> searchFonts(const QString& category, const QString& subset = "latin");

    static QStringList fontCategories();
    static QStringList fontSubsets();

private:
    QString apiKey_;
    QString provider_ = QStringLiteral("googlefonts");

    QVector<FontInfo> parseFontsResponse(const QJsonDocument& doc);
};

class IconFinderApi {
public:
    explicit IconFinderApi(QString apiKey = QString());

    [[nodiscard]] QString provider() const { return provider_; }
    void setProvider(const QString& provider);

    // IconFinder API (free tier available)
    QVector<IconData> searchIcons(const QString& query, int page = 1, int limit = 20);
    IconData getIconDetails(const QString& iconId);
    QString getIconSvgUrl(const QString& iconId);
    
    // Alternative: Phosphor Icons (MIT, no API key needed)
    QVector<IconData> listPhosphorIcons(const QString& category = QString());
    QString getPhosphorSvg(const QString& iconName);

    // Alternative: Feather Icons (MIT, no API key needed)
    QVector<IconData> listFeatherIcons();
    QString getFeatherSvg(const QString& iconName);

    static QStringList iconCategories();

private:
    QString apiKey_;
    QString provider_ = QStringLiteral("iconfinder");

    QVector<IconData> parseIconFinderResponse(const QJsonDocument& doc);
    QVector<IconData> parsePhosphorIcons(const QJsonDocument& doc);
};

class UnsplashApi {
public:
    explicit UnsplashApi(QString accessKey);

    [[nodiscard]] QString provider() const { return provider_; }
    void setProvider(const QString& provider);

    struct PhotoResult {
        QString id;
        QString description;
        QString url;
        QString downloadUrl;
        QString downloadLocationUrl;
        QString photographer;
        int width = 0;
        int height = 0;
        QColor dominantColor;
    };

    QVector<PhotoResult> searchPhotos(const QString& query, int page = 1, int perPage = 20);
    PhotoResult getPhotoDetails(const QString& photoId);
    QString getPhotoDownloadUrl(const QString& photoId, const QString& size = "full");

    static QStringList photoSizes(); // thumb, small, medium, large, full

private:
    QString accessKey_;
    QString provider_ = QStringLiteral("unsplash");

    QVector<PhotoResult> parseUnsplashResponse(const QJsonDocument& doc);
};

} // namespace ccos::api
