#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

class QJsonDocument;
class QNetworkAccessManager;
class QUrl;

namespace ccos::api {

enum class CreativeMediaKind {
    Image,
    Audio,
    Video
};

struct CreativeMediaItem {
    QString provider;
    QString id;
    QString title;
    QString creator;
    QString license;
    QString licenseUrl;
    QString sourceUrl;
    QString previewUrl;
    QString downloadUrl;
    QString mimeType;
    int width = 0;
    int height = 0;
    qint64 durationMs = 0;

    [[nodiscard]] bool isUsable() const noexcept {
        return !id.isEmpty() && !downloadUrl.isEmpty();
    }
};

class CreativeMediaApi final : public QObject {
    Q_OBJECT

public:
    explicit CreativeMediaApi(QObject* parent = nullptr);
    ~CreativeMediaApi() override;

    [[nodiscard]] static QStringList supportedProviders();

    void searchOpenverseImages(const QString& query, int page, int pageSize,
                               std::function<void(const QVector<CreativeMediaItem>&)> callback);
    void searchOpenverseAudio(const QString& query, int page, int pageSize,
                              std::function<void(const QVector<CreativeMediaItem>&)> callback);

    // Wikimedia Commons MediaWiki API. Supports image, video and audio pages.
    void searchWikimediaCommons(const QString& query, CreativeMediaKind kind, int limit,
                                std::function<void(const QVector<CreativeMediaItem>&)> callback);

    // Freesound API v2. Requires an application token supplied by the user.
    // The token is sent in an Authorization header and never serialized by CCOS.
    void searchFreesound(const QString& query, const QString& apiToken, int page, int pageSize,
                         std::function<void(const QVector<CreativeMediaItem>&)> callback);

    static QString kindToString(CreativeMediaKind kind);

Q_SIGNALS:
    void resultsReady(const QVector<CreativeMediaItem>& results);
    void errorOccurred(const QString& provider, const QString& message);

private:
    void requestJson(const QUrl& url, const QString& provider,
                     std::function<void(const QJsonDocument&)> parser,
                     const QString& authorizationToken = {});
    static QVector<CreativeMediaItem> parseOpenverse(const QJsonDocument& document,
                                                     CreativeMediaKind kind);
    static QVector<CreativeMediaItem> parseWikimedia(const QJsonDocument& document,
                                                      CreativeMediaKind kind);
    static QVector<CreativeMediaItem> parseFreesound(const QJsonDocument& document);

    QNetworkAccessManager* networkManager_ = nullptr;
};

} // namespace ccos::api
