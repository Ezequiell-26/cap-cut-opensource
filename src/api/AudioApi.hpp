#pragma once
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace ccos::api {

struct AudioTrackResult {
    QString id;
    QString title;
    QString artist;
    QString url;
    QString downloadUrl;
    QString license;
    QString genre;
    double durationSec = 0.0;
    int sampleRate = 0;
};

struct SoundEffectResult {
    QString id;
    QString name;
    QString description;
    QString url;
    QString downloadUrl;
    QString license;
    QString tags;
    double durationSec = 0.0;
};

class AudioApi {
public:
    explicit AudioApi(QString freesoundApiKey = QString(), QString jamendoclientId = QString());

    [[nodiscard]] QString provider() const { return provider_; }
    void setProvider(const QString& provider);

    // FreeSound API
    QVector<SoundEffectResult> searchSoundEffects(const QString& query, int page = 1, int pageSize = 20);
    SoundEffectResult getSoundEffectDetails(const QString& soundId);
    QString downloadSoundEffect(const QString& soundId);

    // Jamendo API (music for background)
    QVector<AudioTrackResult> searchMusic(const QString& query, const QString& genre = QString(), 
                                          int page = 1, int limit = 20);
    AudioTrackResult getMusicDetails(const QString& trackId);
    QString downloadMusic(const QString& trackId);

    // Internet Archive Audio
    QVector<AudioTrackResult> searchArchiveAudio(const QString& query, int page = 1, int limit = 20);

    static QStringList supportedProviders();
    static QStringList audioGenres();

private:
    QString freesoundApiKey_;
    QString jamendoclientId_;
    QString provider_ = QStringLiteral("freesound");

    QVector<SoundEffectResult> parseFreeSoundResponse(const QJsonDocument& doc);
    QVector<AudioTrackResult> parseJamendoResponse(const QJsonDocument& doc);
    QVector<AudioTrackResult> parseArchiveResponse(const QJsonDocument& doc);
};

} // namespace ccos::api
