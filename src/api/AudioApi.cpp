#include "AudioApi.hpp"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QUrlQuery>
#include <QDebug>

namespace ccos::api {

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
    QEventLoop loop;

    QUrl url("https://freesound.org/apiv2/search/text/");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("query", query);
    queryBuilder.addQueryItem("page", QString::number(page));
    queryBuilder.addQueryItem("page_size", QString::number(pageSize));
    queryBuilder.addQueryItem("fields", "id,name,description,duration,download_url,preview_url,license,tags");
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Token " + freesoundApiKey_).toUtf8());

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "FreeSound API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseFreeSoundResponse(QJsonDocument::fromJson(data));
}

SoundEffectResult AudioApi::getSoundEffectDetails(const QString& soundId) {
    if (freesoundApiKey_.isEmpty()) {
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(QString("https://freesound.org/apiv2/sounds/%1/").arg(soundId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Token " + freesoundApiKey_).toUtf8());

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "FreeSound API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    SoundEffectResult result;
    result.id = obj["id"].toVariant().toString();
    result.name = obj["name"].toString();
    result.description = obj["description"].toString();
    result.url = obj["url"].toString();
    result.downloadUrl = obj["download_url"].toString();
    result.license = obj["license"].toString();
    
    QJsonArray tagsArray = obj["tags"].toArray();
    QStringList tags;
    for (const auto& tag : tagsArray) {
        tags.append(tag.toString());
    }
    result.tags = tags.join(", ");
    result.durationSec = obj["duration"].toDouble(0.0);

    return result;
}

QString AudioApi::downloadSoundEffect(const QString& soundId) {
    // Returns the download URL - actual download handled by MediaImporter
    QUrl url(QString("https://freesound.org/download/sound/%1/").arg(soundId));
    return url.toString();
}

QVector<AudioTrackResult> AudioApi::searchMusic(const QString& query, const QString& genre, int page, int limit) {
    if (jamendoclientId_.isEmpty()) {
        qWarning() << "Jamendo client ID not configured";
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://api.jamendo.com/rest/3.0/tracks/");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("client_id", jamendoclientId_);
    queryBuilder.addQueryItem("format", "json");
    queryBuilder.addQueryItem("limit", QString::number(limit));
    queryBuilder.addQueryItem("offset", QString::number((page - 1) * limit));
    
    if (!query.isEmpty()) {
        queryBuilder.addQueryItem("search", query);
    }
    if (!genre.isEmpty()) {
        queryBuilder.addQueryItem("musicgroup", genre);
    }
    
    queryBuilder.addQueryItem("include", "musicinfo");
    queryBuilder.addQueryItem("audioformat", "mp32");
    
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Jamendo API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseJamendoResponse(QJsonDocument::fromJson(data));
}

AudioTrackResult AudioApi::getMusicDetails(const QString& trackId) {
    if (jamendoclientId_.isEmpty()) {
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://api.jamendo.com/rest/3.0/tracks/");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("client_id", jamendoclientId_);
    queryBuilder.addQueryItem("format", "json");
    queryBuilder.addQueryItem("id", trackId);
    queryBuilder.addQueryItem("include", "musicinfo");
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Jamendo API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QJsonArray results = obj["results"].toArray();

    if (results.isEmpty()) {
        return {};
    }

    return parseJamendoResponse(doc).first();
}

QString AudioApi::downloadMusic(const QString& trackId) {
    if (jamendoclientId_.isEmpty()) {
        return QString();
    }

    // Get audiodownload URL
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://api.jamendo.com/rest/3.0/tracks/");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("client_id", jamendoclientId_);
    queryBuilder.addQueryItem("format", "json");
    queryBuilder.addQueryItem("id", trackId);
    queryBuilder.addQueryItem("audiodownload", "1");
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Jamendo API error:" << reply->errorString();
        reply->deleteLater();
        return QString();
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QJsonArray results = obj["results"].toArray();

    if (results.isEmpty()) {
        return QString();
    }

    return results[0].toObject()["audiodownload"].toString();
}

QVector<AudioTrackResult> AudioApi::searchArchiveAudio(const QString& query, int page, int limit) {
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://archive.org/advancedsearch.php");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("q", QString("(mediatype:audio OR collection:audio_music) AND (%1)").arg(query));
    queryBuilder.addQueryItem("fl[]", "identifier,title,creator,date,length,licenseurl");
    queryBuilder.addQueryItem("rows", QString::number(limit));
    queryBuilder.addQueryItem("page", QString::number(page));
    queryBuilder.addQueryItem("output", "json");
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Internet Archive API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseArchiveResponse(QJsonDocument::fromJson(data));
}

QVector<SoundEffectResult> AudioApi::parseFreeSoundResponse(const QJsonDocument& doc) {
    QVector<SoundEffectResult> results;
    QJsonObject obj = doc.object();
    QJsonArray soundsArray = obj["results"].toArray();

    for (const auto& soundValue : soundsArray) {
        QJsonObject soundObj = soundValue.toObject();
        
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
        for (const auto& tag : tagsArray) {
            tags.append(tag.toString());
        }
        result.tags = tags.join(", ");

        results.append(result);
    }

    return results;
}

QVector<AudioTrackResult> AudioApi::parseJamendoResponse(const QJsonDocument& doc) {
    QVector<AudioTrackResult> results;
    QJsonObject obj = doc.object();
    QJsonArray tracksArray = obj["results"].toArray();

    for (const auto& trackValue : tracksArray) {
        QJsonObject trackObj = trackValue.toObject();
        
        AudioTrackResult result;
        result.id = trackObj["id"].toVariant().toString();
        result.title = trackObj["name"].toString();
        result.artist = trackObj["artist_name"].toString();
        result.url = trackObj["shareurl"].toString();
        result.downloadUrl = trackObj["audiodownload"].toString();
        result.license = trackObj["license_ccurl"].toString();
        result.durationSec = trackObj["duration"].toDouble(0.0);
        
        QJsonObject musicInfo = trackObj["musicinfo"].toObject();
        if (!musicInfo.isEmpty()) {
            result.genre = musicInfo["tag"].toString();
        }

        results.append(result);
    }

    return results;
}

QVector<AudioTrackResult> AudioApi::parseArchiveResponse(const QJsonDocument& doc) {
    QVector<AudioTrackResult> results;
    QJsonObject obj = doc.object();
    QJsonObject responseObj = obj["response"].toObject();
    QJsonArray docsArray = responseObj["docs"].toArray();

    for (const auto& docValue : docsArray) {
        QJsonObject docObj = docValue.toObject();
        
        AudioTrackResult result;
        result.id = docObj["identifier"].toString();
        result.title = docObj["title"].toString();
        
        QVariant creatorVar = docObj["creator"].toVariant();
        if (creatorVar.canConvert<QStringList>()) {
            result.artist = creatorVar.toStringList().join(", ");
        } else {
            result.artist = creatorVar.toString();
        }
        
        result.url = QString("https://archive.org/details/%1").arg(result.id);
        result.downloadUrl = QString("https://archive.org/download/%1").arg(result.id);
        result.license = docObj["licenseurl"].toString();
        result.durationSec = docObj["length"].toDouble(0.0);

        results.append(result);
    }

    return results;
}

} // namespace ccos::api
