#include "api/NasaMediaApi.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace ccos::api {
namespace {
constexpr int kTimeoutMs = 20'000;
constexpr qint64 kMaxResponseBytes = 2 * 1024 * 1024;
}

NasaMediaApi::NasaMediaApi(QObject* parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this)) {}

NasaMediaApi::~NasaMediaApi() = default;

void NasaMediaApi::setApiKey(const QString& apiKey) {
    const QString trimmed = apiKey.trimmed();
    if (!trimmed.isEmpty()) apiKey_ = trimmed;
}

void NasaMediaApi::fetchApod(
    const QString& date,
    std::function<void(const CreativeMediaItem&)> callback) {
    QUrl url(QStringLiteral("https://api.nasa.gov/planetary/apod"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("api_key"), apiKey_);
    if (!date.trimmed().isEmpty()) query.addQueryItem(QStringLiteral("date"), date.trimmed());
    query.addQueryItem(QStringLiteral("thumbs"), QStringLiteral("true"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("CCOS/0.7 NASA-media-client"));
    request.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = networkManager_->get(request);
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->start(kTimeoutMs);

    QPointer<NasaMediaApi> self(this);
    QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [reply, callback = std::move(callback), self, timer]() mutable {
        timer->stop();
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            if (self) emit self->errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        if (payload.size() > kMaxResponseBytes) {
            if (self) emit self->errorOccurred(QStringLiteral("NASA response exceeded safety limit"));
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            if (self) emit self->errorOccurred(parseError.errorString());
            reply->deleteLater();
            return;
        }

        const CreativeMediaItem item = parseApod(document);
        if (!item.isUsable()) {
            if (self) emit self->errorOccurred(QStringLiteral("NASA APOD response did not contain usable media"));
            reply->deleteLater();
            return;
        }
        if (callback) callback(item);
        if (self) emit self->mediaReady(item);
        reply->deleteLater();
    });
}

CreativeMediaItem NasaMediaApi::parseApod(const QJsonDocument& document) {
    CreativeMediaItem item;
    if (!document.isObject()) return item;

    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("media_type")).toString() != QStringLiteral("image") &&
        object.value(QStringLiteral("media_type")).toString() != QStringLiteral("video")) {
        return item;
    }

    item.provider = QStringLiteral("nasa_apod");
    item.id = object.value(QStringLiteral("date")).toString();
    item.title = object.value(QStringLiteral("title")).toString();
    item.creator = object.value(QStringLiteral("copyright")).toString();
    item.license = QStringLiteral("NASA media terms; verify item-specific rights");
    item.sourceUrl = object.value(QStringLiteral("url")).toString();
    item.downloadUrl = object.value(QStringLiteral("hdurl")).toString();
    if (item.downloadUrl.isEmpty()) item.downloadUrl = item.sourceUrl;
    item.previewUrl = object.value(QStringLiteral("url")).toString();
    item.mimeType = object.value(QStringLiteral("media_type")).toString() == QStringLiteral("image")
        ? QStringLiteral("image/*") : QStringLiteral("video/*");

    if (item.id.isEmpty() || item.downloadUrl.isEmpty()) item = CreativeMediaItem{};
    return item;
}

} // namespace ccos::api
