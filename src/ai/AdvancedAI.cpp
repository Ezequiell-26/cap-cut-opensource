#include "AdvancedAI.hpp"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QUrlQuery>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

namespace ccos::ai {

// ==================== Local AI Provider (Ollama, LM Studio, etc.) ====================

LocalAIProvider::LocalAIProvider(QString baseUrl)
    : baseUrl_(std::move(baseUrl))
{
}

void LocalAIProvider::setProvider(const QString& provider) {
    provider_ = provider;
    
    if (provider_ == "ollama") {
        baseUrl_ = "http://localhost:11434/v1";
    } else if (provider_ == "lmstudio") {
        baseUrl_ = "http://localhost:1234/v1";
    } else if (provider_ == "localai") {
        baseUrl_ = "http://localhost:8080/v1";
    }
}

void LocalAIProvider::setBaseUrl(const QString& url) {
    baseUrl_ = url;
}

TextGenerationResult LocalAIProvider::generateText(const QString& prompt,
                                                   const QString& systemPrompt,
                                                   int maxTokens,
                                                   double temperature) {
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(baseUrl_ + "/completions");
    
    QJsonObject requestBody;
    requestBody["model"] = "llama3.2"; // Default model, should be configurable
    requestBody["prompt"] = prompt;
    requestBody["max_tokens"] = maxTokens;
    requestBody["temperature"] = temperature;
    
    if (!systemPrompt.isEmpty()) {
        QJsonArray messages;
        QJsonObject systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = systemPrompt;
        messages.append(systemMessage);
        
        QJsonObject userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = prompt;
        messages.append(userMessage);
        
        requestBody.remove("prompt");
        requestBody["messages"] = messages;
        url = QUrl(baseUrl_ + "/chat/completions");
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.post(request, QJsonDocument(requestBody).toJson());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Local AI API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseCompletionResponse(QJsonDocument::fromJson(data));
}

TextGenerationResult LocalAIProvider::chatCompletion(const QVector<QPair<QString, QString>>& messages,
                                                     int maxTokens) {
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(baseUrl_ + "/chat/completions");
    
    QJsonArray messagesArray;
    for (const auto& msg : messages) {
        QJsonObject messageObj;
        messageObj["role"] = msg.first;
        messageObj["content"] = msg.second;
        messagesArray.append(messageObj);
    }
    
    QJsonObject requestBody;
    requestBody["model"] = "llama3.2";
    requestBody["messages"] = messagesArray;
    requestBody["max_tokens"] = maxTokens;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.post(request, QJsonDocument(requestBody).toJson());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Local AI API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseCompletionResponse(QJsonDocument::fromJson(data));
}

QStringList LocalAIProvider::listModels() {
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(baseUrl_ + "/models");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Local AI API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QStringList models;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QJsonArray dataArray = obj["data"].toArray();

    for (const auto& modelValue : dataArray) {
        QJsonObject modelObj = modelValue.toObject();
        models.append(modelObj["id"].toString());
    }

    return models;
}

bool LocalAIProvider::isAvailable() {
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(baseUrl_);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    // Timeout after 2 seconds
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(2000);
    
    loop.exec();

    bool available = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    
    return available;
}

TextGenerationResult LocalAIProvider::parseCompletionResponse(const QJsonDocument& doc) {
    TextGenerationResult result;
    QJsonObject obj = doc.object();
    
    if (obj.contains("choices")) {
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices[0].toObject();
            
            if (choice.contains("message")) {
                // Chat completion format
                QJsonObject message = choice["message"].toObject();
                result.text = message["content"].toString();
            } else if (choice.contains("text")) {
                // Completion format
                result.text = choice["text"].toString();
            }
            
            result.confidence = choice.contains("finish_reason") ? 1.0 : 0.5;
        }
    }
    
    if (obj.contains("usage")) {
        QJsonObject usage = obj["usage"].toObject();
        result.tokensUsed = usage["total_tokens"].toInt(0);
    }
    
    if (obj.contains("model")) {
        result.model = obj["model"].toString();
    }

    return result;
}

// ==================== Whisper Local Provider ====================

WhisperLocalProvider::WhisperLocalProvider() {
    // Try to find whisper.cpp or whisper model
    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for (const QString& path : paths) {
        QString modelPath = path + "/models/whisper";
        if (QFile::exists(modelPath)) {
            modelPath_ = modelPath;
            break;
        }
    }
}

TranscriptionResult WhisperLocalProvider::transcribe(const QString& audioFilePath,
                                                     const QString& language,
                                                     bool wordTimestamps) {
    return runWhisperCpp(audioFilePath, "base", language, false);
}

TranscriptionResult WhisperLocalProvider::translate(const QString& audioFilePath) {
    return runWhisperCpp(audioFilePath, "base", "en", true);
}

QStringList WhisperLocalProvider::supportedLanguages() {
    return {
        "en", "zh", "de", "es", "ru", "ko", "fr", "ja", "pt", "tr",
        "pl", "ca", "nl", "ar", "sv", "it", "id", "hi", "fi", "vi",
        "he", "uk", "el", "ms", "cs", "ro", "da", "hu", "ta", "no",
        "th", "ur", "hr", "bg", "lt", "la", "mi", "ml", "cy", "sk",
        "te", "fa", "lv", "bn", "sr", "az", "sl", "kn", "et", "mk",
        "br", "eu", "is", "hy", "ne", "mn", "bs", "kk", "sq", "sw",
        "gl", "mr", "pa", "si", "km", "sn", "yo", "so", "af", "oc",
        "ka", "be", "tg", "sd", "gu", "am", "yi", "lo", "uz", "fo",
        "ht", "ps", "tk", "nn", "mt", "sa", "lb", "my", "bo", "tl",
        "mg", "as", "tt", "haw", "ln", "ha", "ba", "jw", "su"
    };
}

bool WhisperLocalProvider::isWhisperAvailable() {
    // Check if whisper.cpp is installed
    QString whisperPath = QStandardPaths::findExecutable("main", {
        "/usr/local/bin",
        "/usr/bin",
        QDir::homePath() + "/whisper.cpp/bin"
    });
    
    return !whisperPath.isEmpty();
}

TranscriptionResult WhisperLocalProvider::runWhisperCpp(const QString& audioPath,
                                                        const QString& model,
                                                        const QString& language,
                                                        bool translate) {
    TranscriptionResult result;
    
    QString whisperPath = QStandardPaths::findExecutable("main", {
        "/usr/local/bin",
        "/usr/bin",
        QDir::homePath() + "/whisper.cpp/bin"
    });
    
    if (whisperPath.isEmpty()) {
        qWarning() << "Whisper.cpp not found";
        return result;
    }
    
    QProcess process;
    QStringList args;
    args << "-m" << (modelPath_ + "/ggml-" + model + ".bin");
    args << "-f" << audioPath;
    
    if (language != "auto") {
        args << "-l" << language;
    }
    
    if (translate) {
        args << "-t"; // Translate to English
    }
    
    process.start(whisperPath, args);
    process.waitForFinished(300000); // 5 minute timeout
    
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    result.text = output.trimmed();
    result.language = language == "auto" ? "detected" : language;
    result.confidence = 0.9; // Whisper is generally very accurate
    
    // Parse segments if timestamps are enabled
    if (process.exitCode() == 0) {
        // Simplified parsing - in production would parse timestamp format
        result.segments.append({0.0, result.text.length() / 10.0, result.text});
    }
    
    return result;
}

// ==================== Hugging Face Inference API ====================

HuggingFaceInference::HuggingFaceInference(QString apiToken)
    : apiToken_(std::move(apiToken))
{
}

void HuggingFaceInference::setApiToken(const QString& token) {
    apiToken_ = token;
}

QByteArray HuggingFaceInference::generateImage(const QString& prompt,
                                               const QString& modelId,
                                               int width,
                                               int height,
                                               int steps) {
    if (apiToken_.isEmpty()) {
        qWarning() << "Hugging Face API token not configured";
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(QString("https://api-inference.huggingface.co/models/%1").arg(modelId));
    
    QJsonObject requestBody;
    requestBody["inputs"] = prompt;
    
    QJsonObject parameters;
    parameters["width"] = width;
    parameters["height"] = height;
    parameters["num_inference_steps"] = steps;
    requestBody["parameters"] = parameters;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Bearer " + apiToken_).toUtf8());

    auto* reply = manager.post(request, QJsonDocument(requestBody).toJson());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Hugging Face API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray imageData = reply->readAll();
    reply->deleteLater();

    return imageData;
}

ImageAnalysisResult HuggingFaceInference::classifyImage(const QByteArray& imageData,
                                                        const QString& modelId) {
    if (apiToken_.isEmpty()) {
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(QString("https://api-inference.huggingface.co/models/%1").arg(modelId));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Bearer " + apiToken_).toUtf8());

    auto* reply = manager.post(request, imageData);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Hugging Face API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    ImageAnalysisResult result;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray predictions = doc.array();

    for (const auto& predValue : predictions) {
        QJsonObject pred = predValue.toObject();
        result.tags.append(pred["label"].toString());
        
        if (result.description.isEmpty()) {
            result.description = pred["label"].toString();
            result.confidence = pred["score"].toDouble(0.0);
        }
    }

    return result;
}

QVariantMap HuggingFaceInference::classifyText(const QString& text,
                                               const QString& modelId) {
    QVariantMap inputs;
    inputs["inputs"] = text;
    
    QVariantMap result = callInferenceApi(modelId, inputs);
    return result;
}

QVector<QPair<QString, QString>> HuggingFaceInference::extractEntities(const QString& text) {
    QVariantMap inputs;
    inputs["inputs"] = text;
    
    QVariantMap result = callInferenceApi("dbmdz/bert-large-cased-finetuned-conll03-english", inputs);
    
    QVector<QPair<QString, QString>> entities;
    // Parse NER results
    return entities;
}

QString HuggingFaceInference::summarize(const QString& text,
                                        int maxLength,
                                        int minLength) {
    QVariantMap inputs;
    inputs["inputs"] = text;
    
    QVariantMap parameters;
    parameters["max_length"] = maxLength;
    parameters["min_length"] = minLength;
    
    QVariantMap result = callInferenceApi("facebook/bart-large-cnn", inputs, parameters);
    
    if (result.contains("summary_text")) {
        return result["summary_text"].toString();
    }
    
    return QString();
}

QString HuggingFaceInference::translateText(const QString& text,
                                            const QString& sourceLang,
                                            const QString& targetLang) {
    QVariantMap inputs;
    inputs["inputs"] = text;
    
    QString modelId = QString("Helsinki-NLP/opus-mt-%1-%2").arg(sourceLang, targetLang);
    QVariantMap result = callInferenceApi(modelId, inputs);
    
    if (result.contains("translation_text")) {
        return result["translation_text"].toString();
    }
    
    return QString();
}

QStringList HuggingFaceInference::availableModels() {
    return {
        // Text Generation
        "meta-llama/Llama-2-7b-chat-hf",
        "mistralai/Mistral-7B-Instruct-v0.2",
        
        // Image Generation
        "stabilityai/stable-diffusion-xl-base-1.0",
        "runwayml/stable-diffusion-v1-5",
        
        // Image Classification
        "google/vit-base-patch16-224",
        "microsoft/resnet-50",
        
        // Text Classification
        "distilbert-base-uncased-finetuned-sst-2-english",
        
        // Summarization
        "facebook/bart-large-cnn",
        "google-t5/t5-base",
        
        // Translation
        "Helsinki-NLP/opus-mt-en-es",
        "Helsinki-NLP/opus-mt-es-en",
        
        // Named Entity Recognition
        "dbmdz/bert-large-cased-finetuned-conll03-english"
    };
}

QVariantMap HuggingFaceInference::callInferenceApi(const QString& modelId,
                                                   const QVariantMap& inputs,
                                                   const QVariantMap& parameters) {
    if (apiToken_.isEmpty()) {
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(QString("https://api-inference.huggingface.co/models/%1").arg(modelId));
    
    QJsonObject requestBody;
    
    // Convert QVariantMap to QJsonObject
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        requestBody[it.key()] = QJsonValue::fromVariant(it.value());
    }
    
    if (!parameters.isEmpty()) {
        QJsonObject paramsObj;
        for (auto it = parameters.begin(); it != parameters.end(); ++it) {
            paramsObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        requestBody["parameters"] = paramsObj;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Bearer " + apiToken_).toUtf8());

    auto* reply = manager.post(request, QJsonDocument(requestBody).toJson());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Hugging Face API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        if (!array.isEmpty()) {
            return array[0].toObject().toVariantMap();
        }
    } else if (doc.isObject()) {
        return doc.object().toVariantMap();
    }

    return {};
}

} // namespace ccos::ai
