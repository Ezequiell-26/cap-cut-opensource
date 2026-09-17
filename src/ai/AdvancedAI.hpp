#pragma once
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace ccos::ai {

struct TextGenerationResult {
    QString text;
    QString model;
    int tokensUsed = 0;
    double confidence = 0.0;
};

struct TranscriptionResult {
    QString text;
    QString language;
    double confidence = 0.0;
    struct Segment {
        double start = 0.0;
        double end = 0.0;
        QString text;
    };
    QVector<Segment> segments;
};

struct ImageAnalysisResult {
    QString description;
    QStringList tags;
    QString dominantColor;
    struct Object {
        QString label;
        double confidence = 0.0;
        QRectF boundingBox; // x, y, width, height (normalized 0-1)
    };
    QVector<Object> objects;
};

class LocalAIProvider {
public:
    explicit LocalAIProvider(QString baseUrl = "http://localhost:11434/v1");

    [[nodiscard]] QString provider() const { return provider_; }
    void setProvider(const QString& provider); // ollama, lmstudio, localai
    void setBaseUrl(const QString& url);

    // Text Generation (OpenAI-compatible API)
    TextGenerationResult generateText(const QString& prompt, 
                                      const QString& systemPrompt = QString(),
                                      int maxTokens = 512,
                                      double temperature = 0.7);
    
    // Chat completion
    TextGenerationResult chatCompletion(const QVector<QPair<QString, QString>>& messages,
                                        int maxTokens = 512);

    // Model listing
    QStringList listModels();

    // Health check
    bool isAvailable();

private:
    QString baseUrl_;
    QString provider_ = QStringLiteral("ollama");

    TextGenerationResult parseCompletionResponse(const QJsonDocument& doc);
};

class WhisperLocalProvider {
public:
    explicit WhisperLocalProvider();

    // Transcribe audio file
    TranscriptionResult transcribe(const QString& audioFilePath,
                                   const QString& language = "auto",
                                   bool wordTimestamps = true);

    // Translate audio to English
    TranscriptionResult translate(const QString& audioFilePath);

    static QStringList supportedLanguages();
    static bool isWhisperAvailable();

private:
    QString modelPath_;
    
    TranscriptionResult runWhisperCpp(const QString& audioPath, 
                                      const QString& model,
                                      const QString& language,
                                      bool translate);
};

class HuggingFaceInference {
public:
    explicit HuggingFaceInference(QString apiToken = QString());

    [[nodiscard]] QString provider() const { return provider_; }
    void setApiToken(const QString& token);

    // Text-to-Image (Stable Diffusion, etc.)
    QByteArray generateImage(const QString& prompt,
                             const QString& modelId = "stabilityai/stable-diffusion-xl-base-1.0",
                             int width = 1024,
                             int height = 1024,
                             int steps = 30);

    // Image Classification
    ImageAnalysisResult classifyImage(const QByteArray& imageData,
                                      const QString& modelId = "google/vit-base-patch16-224");

    // Text Classification (sentiment, topic, etc.)
    QVariantMap classifyText(const QString& text,
                             const QString& modelId = "distilbert-base-uncased-finetuned-sst-2-english");

    // Named Entity Recognition
    QVector<QPair<QString, QString>> extractEntities(const QString& text);

    // Summarization
    QString summarize(const QString& text,
                      int maxLength = 150,
                      int minLength = 30);

    // Translation
    QString translateText(const QString& text,
                          const QString& sourceLang = "en",
                          const QString& targetLang = "es");

    static QStringList availableModels();

private:
    QString apiToken_;
    QString provider_ = QStringLiteral("huggingface");

    QVariantMap callInferenceApi(const QString& modelId,
                                 const QVariantMap& inputs,
                                 const QVariantMap& parameters = QVariantMap());
};

} // namespace ccos::ai
