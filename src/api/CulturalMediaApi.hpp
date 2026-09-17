#pragma once

#include "api/CreativeMediaApi.hpp"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

class QJsonDocument;
class QNetworkAccessManager;
class QUrl;

namespace ccos::api {

// Public cultural/open-access discovery adapters. The returned item always
// retains provider, creator and rights metadata when the provider exposes it.
class CulturalMediaApi final : public QObject {
    Q_OBJECT

public:
    explicit CulturalMediaApi(QObject* parent = nullptr);
    ~CulturalMediaApi() override;

    [[nodiscard]] static QStringList supportedProviders();

    void searchInternetArchive(const QString& query, int page, int pageSize,
                               std::function<void(const QVector<CreativeMediaItem>&)> callback);

    // Smithsonian Open Access uses api.data.gov credentials. Pass the key
    // supplied by the user; CCOS never persists or logs it.
    void searchSmithsonian(const QString& query, const QString& apiKey, int page, int pageSize,
                           std::function<void(const QVector<CreativeMediaItem>&)> callback);

    // The Met Collection API search returns IDs, so CCOS resolves only a
    // bounded number of object records to obtain image metadata/URLs.
    void searchMet(const QString& query, int limit,
                   std::function<void(const QVector<CreativeMediaItem>&)> callback);

    // Europeana Search API requires a free wskey API key.
    void searchEuropeana(const QString& query, const QString& apiKey, int page, int pageSize,
                         std::function<void(const QVector<CreativeMediaItem>&)> callback);

    void searchLibraryOfCongress(const QString& query, int page, int pageSize,
                                 std::function<void(const QVector<CreativeMediaItem>&)> callback);

    static QVector<CreativeMediaItem> parseInternetArchive(const QJsonDocument& document);
    static QVector<CreativeMediaItem> parseSmithsonian(const QJsonDocument& document);
    static CreativeMediaItem parseMetObject(const QJsonDocument& document);
    static QVector<CreativeMediaItem> parseEuropeana(const QJsonDocument& document);
    static QVector<CreativeMediaItem> parseLibraryOfCongress(const QJsonDocument& document);

Q_SIGNALS:
    void resultsReady(const QVector<CreativeMediaItem>& results);
    void errorOccurred(const QString& provider, const QString& message);

private:
    void requestJson(const QUrl& url, const QString& provider,
                     std::function<void(const QJsonDocument&)> parser,
                     const QString& apiKey = {});

    QNetworkAccessManager* networkManager_ = nullptr;
};

} // namespace ccos::api
