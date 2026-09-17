#include "api/CreativeMediaApi.hpp"

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

namespace ccos::api {
namespace {

constexpr int kRequestTimeoutMs = 20'000;
constexpr qint64 kMaxResponseBytes = 8 * 1024 * 1024;

QNetworkRequest makeRequest(const QUrl& url, const QString& authorizationToken = {}) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("CCOS/0.7 open-media-client"));
    request.setRawHeader("Accept", "application/json");
    if (!authorizationToken.isEmpty()) {
        request.setRawHeader("Authorization", (QStringLiteral("Token ") + authorizationToken).toUtf8());
    }
    return request;
}

QString normalizedTitle(const QString& value, const QString& fallback) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

CreativeMediaItem makeOpenverseItem(const QJsonObject& object, CreativeMediaKind kind) {
    CreativeMediaItem item;
    item.provider = QStringLiteral("openverse");
    item.id = object.value(QStringLiteral("id")).toString();
    item.title = normalizedTitle(object.value(QStringLiteral("title")).toString(), item.id);
    item.creator = object.value(QStringLiteral("creator")).toString();
    item.license = object.value(QStringLiteral("license")).toString();
    item.licenseUrl = object.value(QStringLiteral("license_url")).toString();
    item.sourceUrl = object.value(QStringLiteral("foreign_landing_url")).toString();
    item.previewUrl = object.value(QStringLiteral("thumbnail")).toString();
    item.downloadUrl = object.value(QStringLiteral("url")).toString();
    item.mimeType = object.value(QStringLiteral("mime_type")).toString();
    item.width = object.value(QStringLiteral("width")).toInt();
    item.height = object.value(QStringLiteral("height")).toInt();
    if (kind == CreativeMediaKind::Audio) {
        item.durationMs = object.value(QStringLiteral("duration")).toVariant().toLongLong();
    }
    return item;
}

} // namespace

CreativeMediaApi::CreativeMediaApi(QObject* parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this)) {}

CreativeMediaApi::~CreativeMediaApi() = default;

QStringList CreativeMediaApi::supportedProviders() {
    return {QStringLiteral("openverse"), QStringLiteral("wikimedia_commons"), QStringLiteral("freesound")};
}

QString CreativeMediaApi::kindToString(CreativeMediaKind kind) {
    switch (kind) {
    case CreativeMediaKind::Image: return QStringLiteral("image");
    case CreativeMediaKind::Audio: return QStringLiteral("audio");
    case CreativeMediaKind::Video: return QStringLiteral("video");
    }
    return QStringLiteral("unknown");
}

void CreativeMediaApi::requestJson(const QUrl& url, const QString& provider,
                                   std::function<void(const QJsonDocument&)> parser,
                                   const QString& authorizationToken) {
    auto* reply = networkManager_->get(makeRequest(url, authorizationToken));
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->start(kRequestTimeoutMs);

    QPointer<CreativeMediaApi> self(this);
    QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) reply->abort();
    });

    QObject::connect(reply, &QNetworkReply::finished, this,
                     [reply, provider, parser = std::move(parser), self, timer]() mutable {
        timer->stop();
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

void CreativeMediaApi::searchOpenverseImages(
    const QString& query, int page, int pageSize,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    const int safePage = std::max(1, page);
    const int safePageSize = std::clamp(pageSize, 1, 100);
    QUrl url(QStringLiteral("https://api.openverse.org/v1/images/"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("q"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("page"), QString::number(safePage));
    queryParameters.addQueryItem(QStringLiteral("page_size"), QString::number(safePageSize));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("openverse"), [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseOpenverse(document, CreativeMediaKind::Image);
        if (callback) callback(results);
        emit resultsReady(results);
    });
}

void CreativeMediaApi::searchOpenverseAudio(
    const QString& query, int page, int pageSize,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    const int safePage = std::max(1, page);
    const int safePageSize = std::clamp(pageSize, 1, 100);
    QUrl url(QStringLiteral("https://api.openverse.org/v1/audio/"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("q"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("page"), QString::number(safePage));
    queryParameters.addQueryItem(QStringLiteral("page_size"), QString::number(safePageSize));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("openverse"), [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseOpenverse(document, CreativeMediaKind::Audio);
        if (callback) callback(results);
        emit resultsReady(results);
    });
}

void CreativeMediaApi::searchWikimediaCommons(
    const QString& query, CreativeMediaKind kind, int limit,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    const int safeLimit = std::clamp(limit, 1, 50);
    QUrl url(QStringLiteral("https://commons.wikimedia.org/w/api.php"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("action"), QStringLiteral("query"));
    queryParameters.addQueryItem(QStringLiteral("generator"), QStringLiteral("search"));
    queryParameters.addQueryItem(QStringLiteral("gsrsearch"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("gsrnamespace"), QStringLiteral("6"));
    queryParameters.addQueryItem(QStringLiteral("gsrlimit"), QString::number(safeLimit));
    queryParameters.addQueryItem(QStringLiteral("prop"), QStringLiteral("imageinfo"));
    queryParameters.addQueryItem(QStringLiteral("iiprop"), QStringLiteral("url|size|mime|extmetadata"));
    queryParameters.addQueryItem(QStringLiteral("iiurlwidth"), QStringLiteral("1600"));
    queryParameters.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    queryParameters.addQueryItem(QStringLiteral("formatversion"), QStringLiteral("2"));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("wikimedia_commons"),
                [this, kind, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseWikimedia(document, kind);
        if (callback) callback(results);
        emit resultsReady(results);
    });
}

void CreativeMediaApi::searchFreesound(
    const QString& query, const QString& apiToken, int page, int pageSize,
    std::function<void(const QVector<CreativeMediaItem>&)> callback) {
    if (apiToken.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("freesound"), QStringLiteral("Freesound API token is required"));
        return;
    }

    const int safePage = std::max(1, page);
    const int safePageSize = std::clamp(pageSize, 1, 150);
    QUrl url(QStringLiteral("https://freesound.org/apiv2/search/"));
    QUrlQuery queryParameters;
    queryParameters.addQueryItem(QStringLiteral("query"), query.trimmed());
    queryParameters.addQueryItem(QStringLiteral("page"), QString::number(safePage));
    queryParameters.addQueryItem(QStringLiteral("page_size"), QString::number(safePageSize));
    queryParameters.addQueryItem(QStringLiteral("fields"),
                                  QStringLiteral("id,name,username,license,url,previews,duration,type"));
    queryParameters.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(queryParameters);

    requestJson(url, QStringLiteral("freesound"), [this, callback = std::move(callback)](const QJsonDocument& document) mutable {
        const QVector<CreativeMediaItem> results = parseFreesound(document);
        if (callback) callback(results);
        emit resultsReady(results);
    }, apiToken.trimmed());
}

QVector<CreativeMediaItem> CreativeMediaApi::parseOpenverse(
    const QJsonDocument& document, CreativeMediaKind kind) {
    QVector<CreativeMediaItem> results;
    if (!document.isObject()) return results;

    const QJsonArray values = document.object().value(QStringLiteral("results")).toArray();
    results.reserve(values.size());
    for (const QJsonValue& value : values) {
        const CreativeMediaItem item = makeOpenverseItem(value.toObject(), kind);
        if (item.isUsable()) results.append(item);
    }
    return results;
}

QVector<CreativeMediaItem> CreativeMediaApi::parseWikimedia(
    const QJsonDocument& document, CreativeMediaKind kind) {
    QVector<CreativeMediaItem> results;
    if (!document.isObject()) return results;
    Q_UNUSED(kind);

    const QJsonObject query = document.object().value(QStringLiteral("query")).toObject();
    const QJsonArray pages = query.value(QStringLiteral("pages")).toArray();
    results.reserve(pages.size());

    for (const QJsonValue& value : pages) {
        const QJsonObject page = value.toObject();
        const QJsonArray infoArray = page.value(QStringLiteral("imageinfo")).toArray();
        if (infoArray.isEmpty()) continue;
        const QJsonObject info = infoArray.first().toObject();
        const QJsonObject metadata = info.value(QStringLiteral("extmetadata")).toObject();

        CreativeMediaItem item;
        item.provider = QStringLiteral("wikimedia_commons");
        item.id = QString::number(page.value(QStringLiteral("pageid")).toInteger());
        item.title = normalizedTitle(page.value(QStringLiteral("title")).toString(), item.id);
        item.creator = metadata.value(QStringLiteral("Artist")).toObject().value(QStringLiteral("value")).toString();
        item.license = metadata.value(QStringLiteral("LicenseShortName")).toObject().value(QStringLiteral("value")).toString();
        item.licenseUrl = metadata.value(QStringLiteral("LicenseUrl")).toObject().value(QStringLiteral("value")).toString();
        item.sourceUrl = QStringLiteral("https://commons.wikimedia.org/wiki/") +
                         page.value(QStringLiteral("title")).toString().replace(QLatin1Char(' '), QLatin1Char('_'));
        item.previewUrl = info.value(QStringLiteral("thumburl")).toString();
        item.downloadUrl = info.value(QStringLiteral("url")).toString();
        item.mimeType = info.value(QStringLiteral("mime")).toString();
        item.width = info.value(QStringLiteral("width")).toInt();
        item.height = info.value(QStringLiteral("height")).toInt();
        if (item.mimeType.startsWith(QStringLiteral("audio/")) ||
            item.mimeType.startsWith(QStringLiteral("video/"))) {
            item.durationMs = static_cast<qint64>(info.value(QStringLiteral("duration")).toDouble() * 1000.0);
        }

        if (item.isUsable()) results.append(item);
    }
    return results;
}

QVector<CreativeMediaItem> CreativeMediaApi::parseFreesound(const QJsonDocument& document) {
    QVector<CreativeMediaItem> results;
    if (!document.isObject()) return results;

    const QJsonArray values = document.object().value(QStringLiteral("results")).toArray();
    results.reserve(values.size());
    for (const QJsonValue& value : values) {
        const QJsonObject object = value.toObject();
        CreativeMediaItem item;
        item.provider = QStringLiteral("freesound");
        item.id = QString::number(object.value(QStringLiteral("id")).toInteger());
        item.title = normalizedTitle(object.value(QStringLiteral("name")).toString(), item.id);
        item.creator = object.value(QStringLiteral("username")).toString();
        item.license = object.value(QStringLiteral("license")).toString();
        item.sourceUrl = object.value(QStringLiteral("url")).toString();
        item.mimeType = object.value(QStringLiteral("type")).toString();
        item.durationMs = static_cast<qint64>(object.value(QStringLiteral("duration")).toDouble() * 1000.0);

        const QJsonObject previews = object.value(QStringLiteral("previews")).toObject();
        item.previewUrl = previews.value(QStringLiteral("preview-hq-mp3")).toString();
        if (item.previewUrl.isEmpty()) item.previewUrl = previews.value(QStringLiteral("preview-lq-mp3")).toString();
        // Original downloads require a stronger OAuth permission in Freesound.
        // The preview URL is therefore the intentionally exposed import URL here.
        item.downloadUrl = item.previewUrl;

        if (item.isUsable()) results.append(item);
    }
    return results;
}

} // namespace ccos::api
