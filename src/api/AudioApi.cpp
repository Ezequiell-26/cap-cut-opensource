#include "AudioApi.hpp"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QUrlQuery>
#include <QDebug>

namespace ccos::api {
namespace {
constexpr int kApiTimeoutMs = 20'000;
constexpr qint64 kMaxApiResponseBytes = 8 * 1024 * 1024;

QByteArray waitForReply(QNetworkAccessManager& manager, const QNetworkRequest& request, QString* error) {
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(kApiTimeoutMs);

    QNetworkReply* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply]() {
        if (reply->isRunning()) reply->abort();
        loop.quit();
    });

    loop.exec();
    timeout.stop();

    if (reply->error() != QNetworkReply::NoError) {
        if (error) *error = reply->errorString();
        reply->deleteLater();
        return {};
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();
    if (data.size() > kMaxApiResponseBytes) {
        if (error) *error = QStringLiteral("API response exceeded safety limit");
        return {};
    }
    return data;
}

QJsonDocument parseJson(const QByteArray& data, QString* error) {
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) *error = parseError.errorString();
        return {};
    }
    return doc;
}
}

AudioApi::AudioApi(QString freesoundApiKey, QString jamendoclientId)
    : freesoundApiKey_(std::move(freesoundApiKey))
    , jamendoclientId_(std::move(jamendoclientId))
{
}

void AudioApi::setProvider(const QString& provider) {
    provider_ = provider;
}

QStringList AudioApi::supportedProviders() {
    return {"freesound", "jamendo", "archive"};
}

QStringList AudioApi::audioGenres() {
    return {
        "ambient", "cinematic", "corporate", "electronic", "rock",
        "pop", "jazz", "classical", "hip-hop", "country",
        "folk", "blues", "reggae", "world", "soundtrack"
    };
}

QVector<SoundEffectResult> AudioApi::searchSoundEffects(const QString& query, int page, int pageSize) {
    if (freesoundApiKey_.isEmpty()) {
        qWarning() << "FreeSound API key not configured";
        return {};
    }

    QNetworkAccessManager manager;
    QUrl url(QStringLiteral("https://freesound.org/apiv2/search/"));
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem(QStringLiteral("query"), query.trimmed());
    queryBuilder.addQueryItem(QStringLiteral("page"), QString::number(std::max(1, page)));
    queryBuilder.addQueryItem(QStringLiteral("page_size"), QString::number(std::clamp(pageSize, 1, 150)));
    queryBuilder.addQueryItem(QStringLiteral("fields"), QStringLiteral("id,name,description,duration,url,previews,license,tags"));
    queryBuilder.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.7 audio-client"));
    request.setRawHeader("Authorization", (QStringLiteral("Token ") + freesoundApiKey_).toUtf8());

    QString error;
    const QByteArray data = waitForReply(manager, request, &error);
    if (data.isEmpty()) {
        if (!error.isEmpty()) qWarning() << "FreeSound API error:" << error;
        return {};
    }

    return parseFreeSoundResponse(parseJson(data, &error));
}

SoundEffectResult AudioApi::getSoundEffectDetails(const QString& soundId) {
    if (freesoundApiKey_.isEmpty() || soundId.trimmed().isEmpty()) {
        return {};
    }

    QNetworkAccessManager manager;
    QUrl url(QStringLiteral("https://freesound.org/apiv2/sounds/%1/").arg(soundId.trimmed()));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.7 audio-client"));
    request.setRawHeader("Authorization", (QStringLiteral("Token ") + freesoundApiKey_).toUtf8());

    QString error;
    const QJsonDocument doc = parseJson(waitForReply(manager, request, &error), &error);
    if (doc.isNull()) return {};
    const QJsonObject obj = doc.object();

    SoundEffectResult result;
    result.id = obj["id"].toVariant().toString();
    result.name = obj["name"].toString();
    result.description = obj["description"].toString();
    result.url = obj["url"].toString();
    result.downloadUrl = obj["download_url"].toString();
    result.license = obj["license"].toString();

    QJsonArray tagsArray = obj["tags"].toArray();
    QStringList tags;
    for (const auto& tag : tagsArray) tags.append(tag.toString());
    result.tags = tags.join(", ");
    result.durationSec = obj["duration"].toDouble(0.0);
    return result;
}

QString AudioApi::downloadSoundEffect(const QString& soundId) {
    return QUrl(QStringLiteral("https://freesound.org/download/sound/%1/").arg(soundId.trimmed())).toString();
}

QVector<AudioTrackResult> AudioApi::searchMusic(const QString& query, const QString& genre, int page, int limit) {
    if (jamendoclientId_.isEmpty()) {
        qWarning() << "Jamendo client ID not configured";
        return {};
    }

    QNetworkAccessManager manager;
    QUrl url(QStringLiteral("https://api.jamendo.com/rest/3.0/tracks/"));
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem(QStringLiteral("client_id"), jamendoclientId_);
    queryBuilder.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    queryBuilder.addQueryItem(QStringLiteral("limit"), QString::number(std::clamp(limit, 1, 100)));
    queryBuilder.addQueryItem(QStringLiteral("offset"), QString::number(std::max(0, page - 1) * std::clamp(limit, 1, 100)));
    if (!query.trimmed().isEmpty()) queryBuilder.addQueryItem(QStringLiteral("search"), query.trimmed());
    if (!genre.trimmed().isEmpty()) queryBuilder.addQueryItem(QStringLiteral("musicgroup"), genre.trimmed());
    queryBuilder.addQueryItem(QStringLiteral("include"), QStringLiteral("musicinfo"));
    queryBuilder.addQueryItem(QStringLiteral("audioformat"), QStringLiteral("mp32"));
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.7 audio-client"));
    QString error;
    return parseJamendoResponse(parseJson(waitForReply(manager, request, &error), &error));
}

AudioTrackResult AudioApi::getMusicDetails(const QString& trackId) {
    if (jamendoclientId_.isEmpty() || trackId.trimmed().isEmpty()) return {};

    QNetworkAccessManager manager;
    QUrl url(QStringLiteral("https://api.jamendo.com/rest/3.0/tracks/"));
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem(QStringLiteral("client_id"), jamendoclientId_);
    queryBuilder.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    queryBuilder.addQueryItem(QStringLiteral("id"), trackId.trimmed());
    queryBuilder.addQueryItem(QStringLiteral("include"), QStringLiteral("musicinfo"));
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.7 audio-client"));
    QString error;
    const QJsonDocument doc = parseJson(waitForReply(manager, request, &error), &error);
    const QJsonArray results = doc.object()["results"].toArray();
    return results.isEmpty() ? AudioTrackResult{} : parseJamendoResponse(doc).first();
}

QString AudioApi::downloadMusic(const QString& trackId) {
    if (jamendoclientId_.isEmpty() || trackId.trimmed().isEmpty()) return {};

    QNetworkAccessManager manager;
    QUrl url(QStringLiteral("https://api.jamendo.com/rest/3.0/tracks/"));
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem(QStringLiteral("client_id"), jamendoclientId_);
    queryBuilder.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    queryBuilder.addQueryItem(QStringLiteral("id"), trackId.trimmed());
    queryBuilder.addQueryItem(QStringLiteral("audiodownload"), QStringLiteral("1"));
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.7 audio-client"));
    QString error;
    const QJsonDocument doc = parseJson(waitForReply(manager, request, &error), &error);
    const QJsonArray results = doc.object()["results"].toArray();
    return results.isEmpty() ? QString{} : results.first().toObject()["audiodownload"].toString();
}

QVector<AudioTrackResult> AudioApi::searchArchiveAudio(const QString& query, int page, int limit) {
    QNetworkAccessManager manager;
    QUrl url(QStringLiteral("https://archive.org/advancedsearch.php"));
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem(QStringLiteral("q"), QStringLiteral("(mediatype:audio OR collection:audio_music) AND (%1)").arg(query.trimmed()));
    queryBuilder.addQueryItem(QStringLiteral("fl[]"), QStringLiteral("identifier,title,creator,date,length,licenseurl"));
    queryBuilder.addQueryItem(QStringLiteral("rows"), QString::number(std::clamp(limit, 1, 100)));
    queryBuilder.addQueryItem(QStringLiteral("page"), QString::number(std::max(1, page)));
    queryBuilder.addQueryItem(QStringLiteral("output"), QStringLiteral("json"));
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS/0.7 audio-client"));
    QString error;
    return parseArchiveResponse(parseJson(waitForReply(manager, request, &error), &error));
}

QVector<SoundEffectResult> AudioApi::parseFreeSoundResponse(const QJsonDocument& doc) {
    QVector<SoundEffectResult> results;
    const QJsonArray soundsArray = doc.object()["results"].toArray();
    for (const auto& soundValue : soundsArray) {
        const QJsonObject soundObj = soundValue.toObject();
        SoundEffectResult result;
        result.id = soundObj["id"].toVariant().toString();
        result.name = soundObj["name"].toString();
        result.description = soundObj["description"].toString();
        result.url = soundObj["url"].toString();
        result.downloadUrl = soundObj["download_url"].toString();
        result.license = soundObj["license"].toString();
        result.durationSec = soundObj["duration"].toDouble(0.0);
        QJsonArray tagsArray = soundObj["tags"].toArray();
        QStringList tags;
        for (const auto& tag : tagsArray) tags.append(tag.toString());
        result.tags = tags.join(", ");
        results.append(result);
    }
    return results;
}

QVector<AudioTrackResult> AudioApi::parseJamendoResponse(const QJsonDocument& doc) {
    QVector<AudioTrackResult> results;
    const QJsonArray tracksArray = doc.object()["results"].toArray();
    for (const auto& trackValue : tracksArray) {
        const QJsonObject trackObj = trackValue.toObject();
        AudioTrackResult result;
        result.id = trackObj["id"].toVariant().toString();
        result.title = trackObj["name"].toString();
        result.artist = trackObj["artist_name"].toString();
        result.url = trackObj["shareurl"].toString();
        result.downloadUrl = trackObj["audiodownload"].toString();
        result.license = trackObj["license_ccurl"].toString();
        result.durationSec = trackObj["duration"].toDouble(0.0);
        const QJsonObject musicInfo = trackObj["musicinfo"].toObject();
        if (!musicInfo.isEmpty()) result.genre = musicInfo["tag"].toString();
        results.append(result);
    }
    return results;
}

QVector<AudioTrackResult> AudioApi::parseArchiveResponse(const QJsonDocument& doc) {
    QVector<AudioTrackResult> results;
    const QJsonArray docsArray = doc.object()["response"].toObject()["docs"].toArray();
    for (const auto& docValue : docsArray) {
        const QJsonObject docObj = docValue.toObject();
        AudioTrackResult result;
        result.id = docObj["identifier"].toString();
        result.title = docObj["title"].toString();
        const QVariant creatorVar = docObj["creator"].toVariant();
        result.artist = creatorVar.canConvert<QStringList>() ? creatorVar.toStringList().join(", ") : creatorVar.toString();
        result.url = QStringLiteral("https://archive.org/details/%1").arg(result.id);
        result.downloadUrl = QStringLiteral("https://archive.org/download/%1").arg(result.id);
        result.license = docObj["licenseurl"].toString();
        result.durationSec = docObj["length"].toDouble(0.0);
        results.append(result);
    }
    return results;
}

} // namespace ccos::api
