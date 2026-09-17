#include "AccessibilityApi.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrlQuery>
#include <QDateTime>

namespace ccos::api {

AccessibilityApi::AccessibilityApi(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

AccessibilityApi::~AccessibilityApi() = default;

void AccessibilityApi::setGoogleCloudApiKey(const QString& key) {
    m_googleCloudApiKey = key;
}

void AccessibilityApi::setAzureApiKey(const QString& key, const QString& region) {
    m_azureApiKey = key;
    m_azureRegion = region;
    requestAzureAccessToken();
}

void AccessibilityApi::setAwsCredentials(const QString& accessKey, const QString& secretKey, const QString& region) {
    m_awsAccessKey = accessKey;
    m_awsSecretKey = secretKey;
    m_awsRegion = region;
}

void AccessibilityApi::requestAzureAccessToken()
{
    if (m_azureApiKey.isEmpty()) {
        return;
    }
    
    QUrl url(QString("https://%1.api.cognitive.microsoft.com/sts/v1.0/issueToken")
             .arg(m_azureRegion));
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Ocp-Apim-Subscription-Key", m_azureApiKey.toUtf8());
    
    auto* reply = m_networkManager->post(request, QByteArray());
    
    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_azureAccessToken = QString::fromUtf8(reply->readAll());
        }
        reply->deleteLater();
    });
}

// ==================== GOOGLE CLOUD TTS ====================
// Tier gratuito: 4M caracteres/mes

void AccessibilityApi::listAvailableVoices(const QString& languageCode,
                                           std::function<void(const QVector<VoiceProfile>&)> callback)
{
    if (m_googleCloudApiKey.isEmpty()) {
        emit errorOccurred("Google Cloud API key required");
        return;
    }
    
    QUrl url("https://texttospeech.googleapis.com/v1/voices");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("key", m_googleCloudApiKey);
    if (!languageCode.isEmpty()) {
        queryBuilder.addQueryItem("languageCodes", languageCode);
    }
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QVector<VoiceProfile> voices;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray items = root["voices"].toArray();
            
            for (const QJsonValue& val : items) {
                QJsonObject item = val.toObject();
                
                VoiceProfile voice;
                QStringList languageCodes = item["languageCodes"].toArray().toVariantList().toStringList();
                voice.id = item["name"].toString();
                voice.name = item["name"].toString();
                voice.language = languageCodes.isEmpty() ? "" : languageCodes.first();
                voice.gender = item["ssmlGender"].toString().toLower();
                voice.provider = "google";
                voice.isNeural = item["naturalSampleRateHertz"].toInt() > 22050;
                
                QJsonArray features = item["supportedFeatures"].toArray();
                for (const QJsonValue& f : features) {
                    voice.supportedFeatures.append(f.toString());
                }
                
                voices.append(voice);
            }
        }
        
        callback(voices);
        emit voicesReady(voices);
        reply->deleteLater();
    });
}

void AccessibilityApi::textToSpeech(const QString& text, const VoiceProfile& voice,
                                    float speed, float pitch, float volume,
                                    std::function<void(const QByteArray&)> callback)
{
    if (m_googleCloudApiKey.isEmpty()) {
        emit errorOccurred("Google Cloud API key required");
        return;
    }
    
    QUrl url("https://texttospeech.googleapis.com/v1/text:synthesize");
    
    QJsonObject requestBody;
    QJsonObject input;
    input["text"] = text;
    requestBody["input"] = input;
    
    QJsonObject voiceConfig;
    voiceConfig["languageCode"] = voice.language;
    voiceConfig["name"] = voice.name;
    voiceConfig["ssmlGender"] = voice.gender.toUpper();
    requestBody["voice"] = voiceConfig;
    
    QJsonObject audioConfig;
    audioConfig["audioEncoding"] = "MP3";
    audioConfig["speakingRate"] = speed;
    audioConfig["pitch"] = pitch;
    audioConfig["volumeGainDb"] = volume;
    audioConfig["sampleRateHertz"] = 24000;
    requestBody["audioConfig"] = audioConfig;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Goog-Api-Key", m_googleCloudApiKey.toUtf8());
    
    auto* reply = m_networkManager->post(request, QJsonDocument(requestBody).toJson());
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QString audioContent = root["audioContent"].toString();
            QByteArray audioData = QByteArray::fromBase64(audioContent.toUtf8());
            
            callback(audioData);
            emit ttsComplete(audioData);
        }
        
        reply->deleteLater();
    });
}

// ==================== GOOGLE CLOUD STT ====================
// Tier gratuito: 60 minutos/mes

void AccessibilityApi::speechToText(const QByteArray& audioData, const QString& languageCode,
                                    bool enableSpeakerDiarization, bool enableProfanityFilter,
                                    std::function<void(const TranscriptionResult&)> callback)
{
    if (m_googleCloudApiKey.isEmpty()) {
        emit errorOccurred("Google Cloud API key required");
        return;
    }
    
    QUrl url("https://speech.googleapis.com/v1/speech:recognize");
    
    QJsonObject requestBody;
    QJsonObject config;
    config["encoding"] = "LINEAR16";
    config["sampleRateHertz"] = 16000;
    config["languageCode"] = languageCode;
    config["enableAutomaticPunctuation"] = true;
    config["enableWordTimeOffsets"] = true;
    
    if (enableSpeakerDiarization) {
        QJsonObject diarizationConfig;
        diarizationConfig["enableSpeakerDiarization"] = true;
        diarizationConfig["minSpeakerCount"] = 1;
        diarizationConfig["maxSpeakerCount"] = 10;
        config["speakerDiarizationConfig"] = diarizationConfig;
    }
    
    config["profanityFilter"] = enableProfanityFilter;
    requestBody["config"] = config;
    
    QJsonObject audio;
    audio["content"] = audioData.toBase64();
    requestBody["audio"] = audio;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Goog-Api-Key", m_googleCloudApiKey.toUtf8());
    
    auto* reply = m_networkManager->post(request, QJsonDocument(requestBody).toJson());
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        TranscriptionResult result;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray results = root["results"].toArray();
            
            QString fullText;
            int totalConfidence = 0;
            int count = 0;
            
            for (const QJsonValue& val : results) {
                QJsonObject item = val.toObject();
                QJsonArray alternatives = item["alternatives"].toArray();
                
                if (!alternatives.isEmpty()) {
                    QJsonObject alt = alternatives[0].toObject();
                    QString transcript = alt["transcript"].toString();
                    int confidence = qRound(alt["confidence"].toDouble() * 100);
                    
                    fullText += transcript + " ";
                    totalConfidence += confidence;
                    count++;
                    
                    // Timestamps de palabras
                    QJsonArray words = alt["words"].toArray();
                    for (const QJsonValue& w : words) {
                        QJsonObject wordObj = w.toObject();
                        int startTime = wordObj["startTime"].toString().replace("s", "").toFloat() * 1000;
                        int endTime = wordObj["endTime"].toString().replace("s", "").toFloat() * 1000;
                        result.timestamps.append(qMakePair(startTime, endTime));
                    }
                }
            }
            
            result.text = fullText.trimmed();
            result.confidence = count > 0 ? totalConfidence / count : 0;
            result.language = languageCode;
            result.isProfanityFiltered = enableProfanityFilter;
        }
        
        callback(result);
        emit sttComplete(result);
        reply->deleteLater();
    });
}

// ==================== GENERACIÓN DE SUBTÍTULOS ====================

void AccessibilityApi::generateSubtitles(const TranscriptionResult& transcription,
                                         const QString& format,
                                         std::function<void(const QString&)> callback)
{
    QString content;
    
    if (format == "webvtt") {
        content = "WEBVTT\n\n";
        int index = 1;
        
        for (int i = 0; i < transcription.timestamps.size(); ++i) {
            int start = transcription.timestamps[i].first;
            int end = transcription.timestamps[i].second;
            
            // Convertir ms a formato HH:MM:SS.mmm
            auto formatTime = [](int ms) -> QString {
                int hours = ms / 3600000;
                int minutes = (ms % 3600000) / 60000;
                int seconds = (ms % 60000) / 1000;
                int millis = ms % 1000;
                return QString("%1:%2:%3.%4")
                    .arg(hours, 2, 10, QChar('0'))
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'))
                    .arg(millis, 3, 10, QChar('0'));
            };
            
            // Dividir texto en segmentos manejables
            QStringList words = transcription.text.split(" ");
            int wordsPerSegment = qMax(1, words.size() / transcription.timestamps.size());
            
            QString segmentText;
            for (int j = 0; j < wordsPerSegment && (i * wordsPerSegment + j) < words.size(); ++j) {
                segmentText += words[i * wordsPerSegment + j] + " ";
            }
            
            if (!segmentText.trimmed().isEmpty()) {
                content += QString("%1\n%2 --> %3\n%4\n\n")
                    .arg(index++)
                    .arg(formatTime(start))
                    .arg(formatTime(end))
                    .arg(segmentText.trimmed());
            }
        }
    } else if (format == "srt") {
        int index = 1;
        
        for (int i = 0; i < transcription.timestamps.size(); ++i) {
            int start = transcription.timestamps[i].first;
            int end = transcription.timestamps[i].second;
            
            auto formatTimeSrt = [](int ms) -> QString {
                int hours = ms / 3600000;
                int minutes = (ms % 3600000) / 60000;
                int seconds = (ms % 60000) / 1000;
                int millis = ms % 1000;
                return QString("%1:%2:%3,%4")
                    .arg(hours, 2, 10, QChar('0'))
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'))
                    .arg(millis, 3, 10, QChar('0'));
            };
            
            content += QString("%1\n%2 --> %3\n%4\n\n")
                .arg(index++)
                .arg(formatTimeSrt(start))
                .arg(formatTimeSrt(end))
                .arg(transcription.text.split(" ").value(i, ""));
        }
    }
    
    callback(content);
    emit subtitlesReady(content);
}

void AccessibilityApi::translateSubtitles(const QString& subtitleText, const QString& sourceLang,
                                          const QString& targetLang,
                                          std::function<void(const QString&)> callback)
{
    // Usar LibreTranslate (ya implementado en LibreTranslateApi)
    // Esta es una implementación alternativa usando Google Translate no oficial
    
    QUrl url("https://translate.googleapis.com/translate_a/t");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("client", "gtx");
    queryBuilder.addQueryItem("sl", sourceLang);
    queryBuilder.addQueryItem("tl", targetLang);
    queryBuilder.addQueryItem("dt", "t");
    queryBuilder.addQueryItem("q", subtitleText);
    url.setQuery(queryBuilder);
    
    auto* reply = m_networkManager->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QString translated;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isArray()) {
            QJsonArray root = doc.array();
            for (const QJsonValue& val : root) {
                if (val.isArray()) {
                    QJsonArray sentence = val.toArray();
                    if (!sentence.isEmpty() && sentence[0].isArray()) {
                        QJsonArray parts = sentence[0].toArray();
                        for (const QJsonValue& part : parts) {
                            if (part.isArray() && !part.toArray().isEmpty()) {
                                translated += part.toArray()[0].toString();
                            }
                        }
                    }
                }
            }
        }
        
        callback(translated);
        reply->deleteLater();
    });
}

// ==================== DESCRIPCIÓN DE IMÁGENES ====================

void AccessibilityApi::describeImage(const QByteArray& imageData,
                                     std::function<void(const QString&)> callback)
{
    // Usar Hugging Face Inference API para BLIP o similar
    QUrl url("https://api-inference.huggingface.co/models/Salesforce/blip-image-captioning-large");
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    
    auto* reply = m_networkManager->post(request, imageData);
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QString description;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isArray() && !doc.array().isEmpty()) {
            QJsonObject item = doc.array()[0].toObject();
            description = item["generated_text"].toString();
        } else if (doc.isObject()) {
            description = doc.object()["generated_text"].toString();
        }
        
        callback(description);
        emit descriptionReady(description);
        reply->deleteLater();
    });
}

void AccessibilityApi::detectTextInImage(const QByteArray& imageData,
                                         std::function<void(const QString&)> callback)
{
    // Usar Hugging Face para OCR con Tesseract o similar
    QUrl url("https://api-inference.huggingface.co/models/tesseract-ocr");
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    
    auto* reply = m_networkManager->post(request, imageData);
    
    connect(reply, &QNetworkReply::finished, this, [reply, callback, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QString text;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        
        if (doc.isArray()) {
            for (const QJsonValue& val : doc.array()) {
                if (val.isObject()) {
                    text += val.toObject()["generated_text"].toString() + "\n";
                }
            }
        }
        
        callback(text.trimmed());
        reply->deleteLater();
    });
}

// ==================== UTILIDADES ====================

QString AccessibilityApi::convertSrtToWebVtt(const QString& srtContent)
{
    QString webvtt = "WEBVTT\n\n";
    
    QStringList lines = srtContent.split("\n");
    for (const QString& line : lines) {
        // Convertir formato de tiempo SRT (,) a WebVTT (.)
        if (line.contains("-->")) {
            webvtt += line.replace(",", ".") + "\n";
        } else {
            webvtt += line + "\n";
        }
    }
    
    return webvtt;
}

QString AccessibilityApi::convertWebVttToSrt(const QString& webvttContent)
{
    QString srt;
    
    QStringList lines = webvttContent.split("\n");
    bool skipHeader = true;
    int index = 1;
    
    for (const QString& line : lines) {
        if (skipHeader && (line.startsWith("WEBVTT") || line.startsWith("Kind:") || 
                          line.startsWith("Language:") || line.trimmed().isEmpty())) {
            continue;
        }
        skipHeader = false;
        
        if (line.contains("-->")) {
            srt += QString::number(index++) + "\n";
            srt += line.replace(".", ",") + "\n";
        } else if (!line.trimmed().isEmpty()) {
            srt += line + "\n";
        }
    }
    
    return srt;
}

QString AccessibilityApi::simplifyTextForDyslexia(const QString& text)
{
    // Simplificar texto para dislexia:
    // - Oraciones más cortas
    // - Palabras más simples
    // - Espaciado aumentado
    
    QString simplified = text;
    
    // Reemplazar palabras complejas con alternativas más simples
    QMap<QString, QString> replacements = {
        {"utilizar", "usar"},
        {"realizar", "hacer"},
        {"efectuar", "hacer"},
        {"adquirir", "comprar"},
        {"finalizar", "terminar"},
        {"iniciar", "empezar"},
        {"obtener", "conseguir"},
        {"requerir", "necesitar"},
        {"proporcionar", "dar"},
        {"demostrar", "mostrar"}
    };
    
    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        simplified = simplified.replace(it.key(), it.value(), Qt::CaseInsensitive);
    }
    
    return simplified;
}

QString AccessibilityApi::addAudioDescriptionMarkers(const QString& script)
{
    // Agregar marcadores para audio descripción entre diálogos
    QString marked = script;
    
    // Insertar marcadores [DESCRIPCIÓN] donde haya pausas significativas
    QStringList paragraphs = script.split("\n\n");
    QString result;
    
    for (int i = 0; i < paragraphs.size(); ++i) {
        result += paragraphs[i];
        
        // Agregar marcador después de cada 3-4 líneas de diálogo
        if (i % 3 == 2 && i < paragraphs.size() - 1) {
            result += "\n\n[AUDIO DESCRIPTION: Describir acción visual importante]\n";
        }
        
        if (i < paragraphs.size() - 1) {
            result += "\n\n";
        }
    }
    
    return result;
}

} // namespace ccos::api
