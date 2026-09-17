#include "api/CulturalMediaApi.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>
#include <memory>

namespace ccos::api {
namespace {

constexpr int kRequestTimeoutMs = 20'000;
constexpr qint64 kMaxResponseBytes = 8 * 1024 * 1024;

QNetworkRequest requestFor(const QUrl& url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.8 open-cultural-media-client"));
    request.setRawHeader("Accept", "application/json");
    return request;
}

bool isBoundedResponse(const QNetworkReply* reply, const QByteArray& payload) {
    if (!reply) return false;
    const QVariant length = reply->header(QNetworkRequest::ContentLengthHeader);
    return (!length.isValid() || length.toLongLong() <= kMaxResponseBytes) && payload.size() <= kMaxResponseBytes;
}

QString firstString(const QJsonObject& object, const QStringList& keys) {
    for (const QString& key : keys) {
        const QString value = object.value(key).toString().trimmed();
        if (!value.isEmpty()) return value;
    }
    return {};
}

QString normalizedTitle(const QString& value, const QString& fallback) {
    const QString title = value.trimmed();
    return title.isEmpty() ? fallback : title;
}

}

CulturalMediaApi::CulturalMediaApi(QObject* parent)
    : QObject(parent), networkManager_(new QNetworkAccessManager(this)) {}

CulturalMediaApi::~CulturalMediaApi() = default;

QStringList CulturalMediaApi::supportedProviders() {
    return {QStringLiteral("internet_archive"), QStringLiteral("smithsonian_open_access"),
            QStringLiteral("met_open_access"), QStringLiteral("europeana"),
            QStringLiteral("library_of_congress")};
}

void CulturalMediaApi::requestJson(const QUrl& url, const QString& provider,
                                   std::function<void(const QJsonDocument&)> parser,
                                   const QString& apiKey) {
    QNetworkRequest request = requestFor(url);
    if (!apiKey.isEmpty()) request.setRawHeader("X-CCOS-Provider-Key", QByteArray("configured"));
    QNetworkReply* reply = networkManager_->get(request);
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->start(kRequestTimeoutMs);
    QObject::connect(timer, &QTimer::timeout, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [reply, provider, parser = std::move(parser), timer]() mutable {
        timer->stop();
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit static_cast<CulturalMediaApi*>(reply->parent())->errorOccurred(provider, reply->errorString());
            reply->deleteLater();
            return;
        }
        CulturalMediaApi* api = qobject_cast<CulturalMediaApi*>(reply->parent());
        if (!isBoundedResponse(reply, payload)) {
            if (api) emit api->errorOccurred(provider, QStringLiteral("Response exceeded safety limit"));
            reply->deleteLater();
            return;
        }
        QJsonParseError error{};
        const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            if (api) emit api->errorOccurred(provider, error.errorString());
            reply->deleteLater();
            return;
        }
        parser(document);
        reply->deleteLater();
    });
}

void CulturalMediaApi::searchInternetArchive(const QString& query, int page, int pageSize,
                                              std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    QUrl url(QStringLiteral("https://archive.org/advancedsearch.php"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("q"), QStringLiteral("title:(%1) OR description:(%1)").arg(query.trimmed()));
    params.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("identifier"));
    params.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("title"));
    params.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("creator"));
    params.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("mediatype"));
    params.addQueryItem(QStringLiteral("page"), QString::number(std::max(1, page)));
    params.addQueryItem(QStringLiteral("rows"), QString::number(std::clamp(pageSize, 1, 50)));
    params.addQueryItem(QStringLiteral("output"), QStringLiteral("json"));
    url.setQuery(params);
    requestJson(url, QStringLiteral("internet_archive"), [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const auto results = parseInternetArchive(document);
        if (callback) callback(results);
        emit resultsReady(results);
    });
}

void CulturalMediaApi::searchSmithsonian(const QString& query, const QString& apiKey, int page, int pageSize,
                                         std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    if (apiKey.trimmed().isEmpty()) { emit errorOccurred(QStringLiteral("smithsonian_open_access"), QStringLiteral("API key is required")); return; }
    QUrl url(QStringLiteral("https://api.si.edu/openaccess/api/v1.0/search"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("q"), query.trimmed());
    params.addQueryItem(QStringLiteral("start"), QString::number((std::max(1, page) - 1) * std::clamp(pageSize, 1, 100)));
    params.addQueryItem(QStringLiteral("rows"), QString::number(std::clamp(pageSize, 1, 100)));
    params.addQueryItem(QStringLiteral("api_key"), apiKey.trimmed());
    url.setQuery(params);
    requestJson(url, QStringLiteral("smithsonian_open_access"), [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const auto results = parseSmithsonian(document);
        if (callback) callback(results);
        emit resultsReady(results);
    }, apiKey.trimmed());
}

void CulturalMediaApi::searchMet(const QString& query, int limit,
                                 std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    QUrl url(QStringLiteral("https://collectionapi.metmuseum.org/public/collection/v1/search"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("q"), query.trimmed());
    params.addQueryItem(QStringLiteral("hasImages"), QStringLiteral("true"));
    url.setQuery(params);
    requestJson(url, QStringLiteral("met_open_access"), [this, limit, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QJsonArray ids = document.object().value(QStringLiteral("objectIDs")).toArray();
        const int count = static_cast<int>(std::min<qsizetype>(static_cast<qsizetype>(std::clamp(limit, 1, 30)), ids.size()));
        auto results = std::make_shared<QVector<CreativeMediaItem>>();
        auto next = std::make_shared<std::function<void(int)>>();
        *next = [this, ids, count, results, next, callback = std::move(callback)](int index) mutable {
            if (index >= count) { if (callback) callback(*results); emit resultsReady(*results); return; }
            const int id = ids.at(index).toInt(-1);
            if (id <= 0) { (*next)(index + 1); return; }
            const QUrl objectUrl(QStringLiteral("https://collectionapi.metmuseum.org/public/collection/v1/objects/%1").arg(id));
            requestJson(objectUrl, QStringLiteral("met_open_access"), [this, results, next, index](const QJsonDocument& object) mutable {
                const auto item = parseMetObject(object);
                if (item.isUsable()) results->append(item);
                (*next)(index + 1);
            });
        };
        (*next)(0);
    });
}

void CulturalMediaApi::searchEuropeana(const QString& query, const QString& apiKey, int page, int pageSize,
                                       std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    if (apiKey.trimmed().isEmpty()) { emit errorOccurred(QStringLiteral("europeana"), QStringLiteral("API key is required")); return; }
    QUrl url(QStringLiteral("https://api.europeana.eu/record/v2/search.json"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("wskey"), apiKey.trimmed());
    params.addQueryItem(QStringLiteral("query"), query.trimmed());
    params.addQueryItem(QStringLiteral("page"), QString::number(std::max(1, page)));
    params.addQueryItem(QStringLiteral("rows"), QString::number(std::clamp(pageSize, 1, 100)));
    params.addQueryItem(QStringLiteral("profile"), QStringLiteral("standard"));
    url.setQuery(params);
    requestJson(url, QStringLiteral("europeana"), [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const auto results = parseEuropeana(document);
        if (callback) callback(results);
        emit resultsReady(results);
    }, apiKey.trimmed());
}

void CulturalMediaApi::searchLibraryOfCongress(const QString& query, int page, int pageSize,
                                                std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    QUrl url(QStringLiteral("https://www.loc.gov/photos/"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("q"), query.trimmed());
    params.addQueryItem(QStringLiteral("sp"), QString::number(std::max(1, page)));
    params.addQueryItem(QStringLiteral("fo"), QStringLiteral("json"));
    params.addQueryItem(QStringLiteral("c"), QString::number(std::clamp(pageSize, 1, 50)));
    params.addQueryItem(QStringLiteral("at"), QStringLiteral("resources,set,results"));
    url.setQuery(params);
    requestJson(url, QStringLiteral("library_of_congress"), [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const auto results = parseLibraryOfCongress(document);
        if (callback) callback(results);
        emit resultsReady(results);
    });
}

QVector<CreativeMediaItem> CulturalMediaApi::parseInternetArchive(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    const QJsonArray docs = document.object().value(QStringLiteral("response")).toObject().value(QStringLiteral("docs")).toArray();
    for (const auto& value : docs) {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("identifier")).toString();
        if (id.isEmpty()) continue;
        CreativeMediaItem item;
        item.provider = QStringLiteral("internet_archive");
        item.id = id;
        item.title = normalizedTitle(object.value(QStringLiteral("title")).toString(), id);
        item.creator = object.value(QStringLiteral("creator")).toString();
        item.sourceUrl = QStringLiteral("https://archive.org/details/%1").arg(id);
        item.previewUrl = item.sourceUrl;
        item.downloadUrl = QStringLiteral("https://archive.org/download/%1/").arg(id);
        item.mimeType = object.value(QStringLiteral("mediatype")).toString();
        item.license = QStringLiteral("item-specific; verify rights");
        if (item.isUsable()) results.append(item);
    }
    return results;
}

QVector<CreativeMediaItem> CulturalMediaApi::parseSmithsonian(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    for (const auto& value : document.object().value(QStringLiteral("response")).toObject().value(QStringLiteral("rows")).toArray()) {
        const QJsonObject content = value.toObject().value(QStringLiteral("content")).toObject();
        const QJsonObject descriptive = content.value(QStringLiteral("descriptiveNonRepeating")).toObject();
        const QString id = firstString(descriptive, {QStringLiteral("record_ID"), QStringLiteral("recordLink")});
        const QJsonObject online = descriptive.value(QStringLiteral("online_media")).toObject();
        const QJsonArray media = online.value(QStringLiteral("media")).toArray();
        const QString mediaUrl = media.isEmpty() ? QString() : media.first().toObject().value(QStringLiteral("content")).toString();
        CreativeMediaItem item;
        item.provider = QStringLiteral("smithsonian_open_access");
        item.id = id;
        item.title = normalizedTitle(descriptive.value(QStringLiteral("title")).toObject().value(QStringLiteral("content")).toString(), id);
        item.sourceUrl = descriptive.value(QStringLiteral("recordLink")).toString();
        item.previewUrl = mediaUrl;
        item.downloadUrl = mediaUrl;
        item.mimeType = QStringLiteral("image/*");
        item.license = QStringLiteral("verify item rights; CC0 when explicitly marked");
        if (item.sourceUrl.isEmpty() && !id.isEmpty()) item.sourceUrl = QStringLiteral("https://www.si.edu/object/%1").arg(id);
        if (item.isUsable()) results.append(item);
    }
    return results;
}

CreativeMediaItem CulturalMediaApi::parseMetObject(const QJsonDocument& document) {
    CreativeMediaItem item;
    if (!document.isObject()) return item;
    const QJsonObject object = document.object();
    const int id = object.value(QStringLiteral("objectID")).toInt(-1);
    if (id <= 0) return item;
    item.provider = QStringLiteral("met_open_access");
    item.id = QString::number(id);
    item.title = normalizedTitle(object.value(QStringLiteral("title")).toString(), item.id);
    item.creator = firstString(object, {QStringLiteral("artistDisplayName"), QStringLiteral("artistRole")});
    item.sourceUrl = object.value(QStringLiteral("objectURL")).toString();
    item.previewUrl = object.value(QStringLiteral("primaryImageSmall")).toString();
    item.downloadUrl = object.value(QStringLiteral("primaryImage")).toString();
    item.mimeType = QStringLiteral("image/jpeg");
    item.license = QStringLiteral("CC0/public-domain image where provided; verify record");
    item.licenseUrl = QStringLiteral("https://www.metmuseum.org/hubs/open-access");
    return item;
}

QVector<CreativeMediaItem> CulturalMediaApi::parseEuropeana(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    for (const auto& value : document.object().value(QStringLiteral("items")).toArray()) {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) continue;
        CreativeMediaItem item;
        item.provider = QStringLiteral("europeana");
        item.id = id;
        item.title = normalizedTitle(object.value(QStringLiteral("title")).toString(), id);
        item.sourceUrl = object.value(QStringLiteral("guid")).toString();
        item.previewUrl = object.value(QStringLiteral("edmPreview")).toString();
        item.downloadUrl = object.value(QStringLiteral("edmIsShownBy")).toString();
        item.mimeType = object.value(QStringLiteral("type")).toString();
        item.license = object.value(QStringLiteral("rights")).toString();
        if (item.isUsable()) results.append(item);
    }
    return results;
}

QVector<CreativeMediaItem> CulturalMediaApi::parseLibraryOfCongress(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    for (const auto& value : document.object().value(QStringLiteral("results")).toArray()) {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) continue;
        CreativeMediaItem item;
        item.provider = QStringLiteral("library_of_congress");
        item.id = id;
        item.title = normalizedTitle(object.value(QStringLiteral("title")).toString(), id);
        item.sourceUrl = id;
        const QJsonArray images = object.value(QStringLiteral("image_url")).toArray();
        if (!images.isEmpty()) {
            item.previewUrl = images.first().toString();
            item.downloadUrl = images.last().toString();
        }
        item.mimeType = QStringLiteral("image/*");
        item.license = QStringLiteral("item-specific; verify rights");
        if (item.isUsable()) results.append(item);
    }
    return results;
}

} // namespace ccos::api
