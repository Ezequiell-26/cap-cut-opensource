#include "api/CulturalMediaApi.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>
#include <memory>

namespace ccos::api {
namespace {

constexpr int kRequestTimeoutMs = 20'000;
constexpr qint64 kMaxResponseBytes = 8 * 1024 * 1024;
constexpr int kMaxMetObjects = 30;

QNetworkRequest makeRequest(const QUrl& url, const QString& apiKey = {}) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("CCOS/0.7 open-cultural-media-client"));
    request.setRawHeader("Accept", "application/json");
    if (!apiKey.isEmpty()) {
        request.setRawHeader("X-CCOS-Provider-Key", QByteArray("configured"));
    }
    return request;
}

QString textValue(const QJsonObject& object, const QString& key) {
    return object.value(key).toString().trimmed();
}

QString firstString(const QJsonObject& object, const QStringList& keys) {
    for (const QString& key : keys) {
        const QString value = textValue(object, key);
        if (!value.isEmpty()) return value;
    }
    return {};
}

QString normalizedTitle(const QString& value, const QString& fallback) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

QString nestedContent(const QJsonObject& object, const QString& parent, const QString& child) {
    return object.value(parent).toObject().value(child).toString().trimmed();
}

QString archiveItemUrl(const QString& identifier) {
    return QStringLiteral("https://archive.org/details/") + identifier;
}

} // namespace

CulturalMediaApi::CulturalMediaApi(QObject* parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this)) {}

CulturalMediaApi::~CulturalMediaApi() = default;

QStringList CulturalMediaApi::supportedProviders() {
    return {
        QStringLiteral("internet_archive"),
        QStringLiteral("smithsonian_open_access"),
        QStringLiteral("met_open_access"),
        QStringLiteral("europeana"),
        QStringLiteral("library_of_congress")
    };
}

void CulturalMediaApi::requestJson(const QUrl& url, const QString& provider,
                                   std::function<void(const QJsonDocument&)> parser,
                                   const QString& apiKey) {
    QNetworkRequest request = makeRequest(url, apiKey);
    auto* reply = networkManager_->get(request);
    reply->setReadBufferSize(static_cast<int>(kMaxResponseBytes + 1));

    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->start(kRequestTimeoutMs);

    QPointer<CulturalMediaApi> self(this);
    QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) reply->abort();
    });

    QObject::connect(reply, &QNetworkReply::finished, this,
                     [reply, provider, parser = std::move(parser), self, timer]() mutable {
        timer->stop();

        const QVariant contentLength = reply->header(QNetworkRequest::ContentLengthHeader);
        if (contentLength.isValid() && contentLength.toLongLong() > kMaxResponseBytes) {
            if (self) emit self->errorOccurred(provider, QStringLiteral("Response exceeded safety limit"));
            reply->deleteLater();
            return;
        }

        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            if (self) emit self->errorOccurred(provider, reply->errorString());
            reply->deleteLater();
            return;
        }
        if (payload.size() > kMaxResponseBytes) {
            if (self) emit self->errorOccurred(provider, QStringLiteral("Response exceeded safety limit"));
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            if (self) emit self->errorOccurred(provider, parseError.errorString());
            reply->deleteLater();
            return;
        }

        parser(document);
        reply->deleteLater();
    });
}

void CulturalMediaApi::searchInternetArchive(
    const QString& query, int page, int pageSize,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    const int safePage = std::max(1, page);
    const int safePageSize = std::clamp(pageSize, 1, 50);

    QUrl url(QStringLiteral("https://archive.org/advancedsearch.php"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("q"),
                                  QStringLiteral("(title:(%1) OR description:(%1)) AND mediatype:(movies OR audio OR image)")
                                      .arg(query.trimmed()));
    queryParameters.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("identifier"));
    queryParameters.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("title"));
    queryParameters.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("creator"));
    queryParameters.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("description"));
    queryParameters.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("mediatype"));
    queryParameters.addQueryItem(QStringLiteral("page"), QString::number(safePage));
    queryParameters.addQueryItem(QStringLiteral("rows"), QString::number(safePageSize));
    queryParameters.addQueryItem(QStringLiteral("output"), QStringLiteral("json"));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("internet_archive"),
                [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseInternetArchive(document);
        if (callback) callback(results);
        emit resultsReady(results);
    });
}

void CulturalMediaApi::searchSmithsonian(
    const QString& query, const QString& apiKey, int page, int pageSize,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    if (apiKey.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("smithsonian_open_access"),
                           QStringLiteral("Smithsonian API key is required"));
        return;
    }

    const int safePage = std::max(1, page);
    const int safePageSize = std::clamp(pageSize, 1, 100);
    QUrl url(QStringLiteral("https://api.si.edu/openaccess/api/v1.0/search"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("q"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("start"),
                                  QString::number((safePage - 1) * safePageSize));
    queryParameters.addQueryItem(QStringLiteral("rows"), QString::number(safePageSize));
    queryParameters.addQueryItem(QStringLiteral("api_key"), apiKey.trimmed());
    url.setQuery(queryParameters);

    // The Smithsonian documents api.data.gov credentials on this endpoint.
    requestJson(url, QStringLiteral("smithsonian_open_access"),
                [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseSmithsonian(document);
        if (callback) callback(results);
        emit resultsReady(results);
    }, apiKey.trimmed());
}

void CulturalMediaApi::searchMet(
    const QString& query, int limit,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    const int safeLimit = std::clamp(limit, 1, kMaxMetObjects);
    QUrl url(QStringLiteral("https://collectionapi.metmuseum.org/public/collection/v1/search"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("q"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("hasImages"), QStringLiteral("true"));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("met_open_access"),
                [this, safeLimit, callback = std::move(callback)](const QJsonDocument& document) mutable {
        if (!document.isObject()) {
            if (callback) callback({});
            emit resultsReady({});
            return;
        }

        const QJsonArray ids = document.object().value(QStringLiteral("objectIDs")).toArray();
        const int count = std::min(safeLimit, ids.size());
        auto results = std::make_shared<QVector<CreativeMediaItem>>();
        auto next = std::make_shared<std::function<void(int)>>();

        *next = [this, ids, count, results, next, callback = std::move(callback)](int index) mutable {
            if (index >= count) {
                if (callback) callback(*results);
                emit resultsReady(*results);
                return;
            }

            const int objectId = ids.at(index).toInt(-1);
            if (objectId <= 0) {
                (*next)(index + 1);
                return;
            }

            QUrl objectUrl(QStringLiteral("https://collectionapi.metmuseum.org/public/collection/v1/objects/%1")
                               .arg(objectId));
            requestJson(objectUrl, QStringLiteral("met_open_access"),
                        [this, results, next, index](const QJsonDocument& objectDocument) mutable {
                const CreativeMediaItem item = parseMetObject(objectDocument);
                if (item.isUsable()) results->append(item);
                (*next)(index + 1);
            });
        };

        (*next)(0);
    });
}

void CulturalMediaApi::searchEuropeana(
    const QString& query, const QString& apiKey, int page, int pageSize,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    if (apiKey.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("europeana"),
                           QStringLiteral("Europeana API key is required"));
        return;
    }

    const int safePage = std::max(1, page);
    const int safePageSize = std::clamp(pageSize, 1, 100);
    QUrl url(QStringLiteral("https://api.europeana.eu/record/v2/search.json"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("wskey"), apiKey.trimmed());
    queryParameters.addQueryItem(QStringLiteral("query"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("page"), QString::number(safePage));
    queryParameters.addQueryItem(QStringLiteral("rows"), QString::number(safePageSize));
    queryParameters.addQueryItem(QStringLiteral("profile"), QStringLiteral("standard"));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("europeana"),
                [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseEuropeana(document);
        if (callback) callback(results);
        emit resultsReady(results);
    }, apiKey.trimmed());
}

void CulturalMediaApi::searchLibraryOfCongress(
    const QString& query, int page, int pageSize,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    const int safePage = std::max(1, page);
    const int safePageSize = std::clamp(pageSize, 1, 50);
    QUrl url(QStringLiteral("https://www.loc.gov/photos/"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("q"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("sp"), QString::number(safePage));
    queryParameters.addQueryItem(QStringLiteral("fo"), QStringLiteral("json"));
    queryParameters.addQueryItem(QStringLiteral("c"), QString::number(safePageSize));
    queryParameters.addQueryItem(QStringLiteral("at"), QStringLiteral("resources,set,results"));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("library_of_congress"),
                [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseLibraryOfCongress(document);
        if (callback) callback(results);
        emit resultsReady(results);
    });
}

QVector<CreativeMediaItem> CulturalMediaApi::parseInternetArchive(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    if (!document.isObject()) return results;
    const QJsonArray docs = document.object().value(QStringLiteral("response")).toObject()
                                .value(QStringLiteral("docs")).toArray();
    results.reserve(docs.size());

    for (const QJsonValue& value : docs) {
        const QJsonObject object = value.toObject();
        const QString id = textValue(object, QStringLiteral("identifier"));
        if (id.isEmpty()) continue;

        CreativeMediaItem item;
        item.provider = QStringLiteral("internet_archive");
        item.id = id;
        item.title = firstString(object, {QStringLiteral("title"), QStringLiteral("identifier")});
        item.creator = textValue(object, QStringLiteral("creator"));
        item.license = QStringLiteral("provider/item-specific");
        item.sourceUrl = archiveItemUrl(id);
        item.previewUrl = item.sourceUrl;
        item.downloadUrl = QStringLiteral("https://archive.org/download/") + id + QLatin1Char('/');
        item.mimeType = textValue(object, QStringLiteral("mediatype"));

        if (item.isUsable()) results.append(item);
    }
    return results;
}

QVector<CreativeMediaItem> CulturalMediaApi::parseSmithsonian(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    if (!document.isObject()) return results;
    const QJsonArray rows = document.object().value(QStringLiteral("response")).toObject()
                                .value(QStringLiteral("rows")).toArray();
    results.reserve(rows.size());

    for (const QJsonValue& value : rows) {
        const QJsonObject row = value.toObject();
        const QJsonObject content = row.value(QStringLiteral("content")).toObject();
        const QJsonObject descriptive = content.value(QStringLiteral("descriptiveNonRepeating")).toObject();
        const QString id = firstString(descriptive, {QStringLiteral("record_ID"), QStringLiteral("recordLink")});

        const QJsonObject onlineMedia = descriptive.value(QStringLiteral("online_media")).toObject();
        const QJsonArray media = onlineMedia.value(QStringLiteral("media")).toArray();
        QString mediaUrl;
        if (!media.isEmpty()) mediaUrl = media.first().toObject().value(QStringLiteral("content")).toString().trimmed();

        CreativeMediaItem item;
        item.provider = QStringLiteral("smithsonian_open_access");
        item.id = id;
        item.title = normalizedTitle(nestedContent(descriptive, QStringLiteral("title"), QStringLiteral("content")), id);
        item.sourceUrl = descriptive.value(QStringLiteral("recordLink")).toString().trimmed();
        item.previewUrl = mediaUrl;
        item.downloadUrl = mediaUrl;
        item.license = QStringLiteral("CC0 metadata / verify item rights");
        item.licenseUrl = QStringLiteral("https://www.si.edu/openaccess");
        item.mimeType = QStringLiteral("image/*");
        if (item.sourceUrl.isEmpty() && !id.isEmpty()) {
            item.sourceUrl = QStringLiteral("https://www.si.edu/object/") + id;
        }

        if (item.isUsable()) results.append(item);
    }
    return results;
}

CreativeMediaItem CulturalMediaApi::parseMetObject(const QJsonDocument& document) {
    CreativeMediaItem item;
    if (!document.isObject()) return item;
    const QJsonObject object = document.object();
    const QString id = QString::number(object.value(QStringLiteral("objectID")).toInt(-1));
    if (id == QStringLiteral("-1") || id == QStringLiteral("0")) return item;

    item.provider = QStringLiteral("met_open_access");
    item.id = id;
    item.title = normalizedTitle(textValue(object, QStringLiteral("title")), item.id);
    item.creator = firstString(object, {QStringLiteral("artistDisplayName"), QStringLiteral("artistRole")});
    item.license = QStringLiteral("CC0 / public-domain image; verify item record");
    item.licenseUrl = QStringLiteral("https://www.metmuseum.org/hubs/open-access");
    item.sourceUrl = textValue(object, QStringLiteral("objectURL"));
    item.previewUrl = textValue(object, QStringLiteral("primaryImageSmall"));
    item.downloadUrl = textValue(object, QStringLiteral("primaryImage"));
    item.mimeType = QStringLiteral("image/jpeg");

    return item;
}

QVector<CreativeMediaItem> CulturalMediaApi::parseEuropeana(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    if (!document.isObject()) return results;
    const QJsonArray items = document.object().value(QStringLiteral("items")).toArray();
    results.reserve(items.size());

    for (const QJsonValue& value : items) {
        const QJsonObject object = value.toObject();
        CreativeMediaItem item;
        item.provider = QStringLiteral("europeana");
        item.id = textValue(object, QStringLiteral("id"));
        item.title = normalizedTitle(textValue(object, QStringLiteral("title")), item.id);
        item.creator = textValue(object, QStringLiteral("dcCreator"));
        item.license = textValue(object, QStringLiteral("rights"));
        item.sourceUrl = firstString(object, {QStringLiteral("edmIsShownAt"), QStringLiteral("guid")});
        item.previewUrl = textValue(object, QStringLiteral("edmPreview"));
        item.downloadUrl = item.previewUrl.isEmpty() ? item.sourceUrl : item.previewUrl;
        item.mimeType = textValue(object, QStringLiteral("type"));

        if (item.isUsable()) results.append(item);
    }
    return results;
}

QVector<CreativeMediaItem> CulturalMediaApi::parseLibraryOfCongress(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    if (!document.isObject()) return results;
    const QJsonArray values = document.object().value(QStringLiteral("results")).toArray();
    results.reserve(values.size());

    for (const QJsonValue& value : values) {
        const QJsonObject object = value.toObject();
        const QString id = textValue(object, QStringLiteral("id"));
        if (id.isEmpty()) continue;

        CreativeMediaItem item;
        item.provider = QStringLiteral("library_of_congress");
        item.id = id;
        item.title = normalizedTitle(textValue(object, QStringLiteral("title")), id);
        item.creator = textValue(object, QStringLiteral("contributor"));
        item.license = QStringLiteral("item-specific / verify rights statement");
        item.sourceUrl = id.startsWith(QStringLiteral("http")) ? id : QStringLiteral("https://www.loc.gov") + id;

        const QJsonArray imageUrls = object.value(QStringLiteral("image_url")).toArray();
        if (!imageUrls.isEmpty()) {
            item.previewUrl = imageUrls.first().toString().trimmed();
            item.downloadUrl = item.previewUrl;
            item.mimeType = QStringLiteral("image/*");
        }

        if (item.isUsable()) results.append(item);
    }
    return results;
}

} // namespace ccos::api
