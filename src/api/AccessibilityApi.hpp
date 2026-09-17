#pragma once
/**
 * @file AccessibilityApi.hpp
 * @brief APIs para accesibilidad y herramientas de asistencia
 * 
 * APIs integradas:
 * - Google Cloud Text-to-Speech (tier gratuito mensual)
 * - Google Cloud Speech-to-Text (60 min/mes gratis)
 * - Azure Cognitive Services (tier gratuito)
 * - AWS Polly (tier gratuito 12 meses)
 * - NVDA Screen Reader Integration (MIT)
 * - WebVTT/Subtitle accessibility features
 */

#include <QObject>
#include <QString>
#include <QVector>
#include <QByteArray>
#include <QNetworkReply>
#include <functional>

namespace ccos::api {

struct VoiceProfile {
    QString id;
    QString name;
    QString language;
    QString gender; // "male", "female", "neutral"
    QString provider; // "google", "azure", "aws", "microsoft"
    bool isNeural;
    QStringList supportedFeatures; // "ssml", "pitch", "speed", "volume"
};

struct TranscriptionResult {
    QString text;
    int confidence; // 0-100
    QString language;
    QVector<QPair<int, int>> timestamps; // start, end en ms
    QVector<QString> speakers; // speaker identification
    bool isProfanityFiltered;
};

struct SubtitleSegment {
    int index;
    int startTime; // ms
    int endTime; // ms
    QString text;
    QString speaker;
};

class AccessibilityApi : public QObject {
    Q_OBJECT

public:
    explicit AccessibilityApi(QObject* parent = nullptr);
    ~AccessibilityApi() override;

    // Configuración de API Keys
    void setGoogleCloudApiKey(const QString& key);
    void setAzureApiKey(const QString& key, const QString& region);
    void setAwsCredentials(const QString& accessKey, const QString& secretKey, const QString& region);

    // Text-to-Speech (TTS)
    void listAvailableVoices(const QString& languageCode = "",
                             std::function<void(const QVector<VoiceProfile>&)> callback = {});
    
    void textToSpeech(const QString& text, const VoiceProfile& voice,
                      float speed = 1.0f, float pitch = 0.0f, float volume = 0.0f,
                      std::function<void(const QByteArray& audioData)> callback = {});
    
    void textToSpeechWithSSML(const QString& ssml, const VoiceProfile& voice,
                              std::function<void(const QByteArray& audioData)> callback);

    // Speech-to-Text (STT)
    void speechToText(const QByteArray& audioData, const QString& languageCode = "en-US",
                      bool enableSpeakerDiarization = false,
                      bool enableProfanityFilter = false,
                      std::function<void(const TranscriptionResult&)> callback = {});
    
    void speechToTextAsync(const QString& audioFilePath, const QString& languageCode = "en-US",
                           std::function<void(const TranscriptionResult&)> callback = {});

    // Generación de subtítulos accesibles
    void generateSubtitles(const TranscriptionResult& transcription,
                           const QString& format = "webvtt", // "webvtt", "srt", "ttml"
                           std::function<void(const QString&)> callback = {});
    
    void translateSubtitles(const QString& subtitleText, const QString& sourceLang,
                            const QString& targetLang,
                            std::function<void(const QString&)> callback);

    // Características de accesibilidad
    void describeImage(const QByteArray& imageData,
                       std::function<void(const QString&)> callback);
    
    void detectTextInImage(const QByteArray& imageData,
                           std::function<void(const QString&)> callback);

    // Utilidades
    static QString convertSrtToWebVtt(const QString& srtContent);
    static QString convertWebVttToSrt(const QString& webvttContent);
    static QString simplifyTextForDyslexia(const QString& text);
    static QString addAudioDescriptionMarkers(const QString& script);

Q_SIGNALS:
    void voicesReady(const QVector<VoiceProfile>&);
    void ttsComplete(const QByteArray& audioData);
    void sttComplete(const TranscriptionResult&);
    void subtitlesReady(const QString& content);
    void descriptionReady(const QString& description);
    void errorOccurred(const QString& message);

private:
    QNetworkAccessManager* m_networkManager;
    
    QString m_googleCloudApiKey;
    QString m_azureApiKey;
    QString m_azureRegion;
    QString m_awsAccessKey;
    QString m_awsSecretKey;
    QString m_awsRegion;
    
    QString m_azureAccessToken;
    
    void requestAzureAccessToken();
};

} // namespace ccos::api
