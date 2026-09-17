#pragma once
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace ccos::api {

struct TranslationResult {
    QString sourceLanguage;
    QString targetLanguage;
    QString originalText;
    QString translatedText;
    QString provider;
};

class LibreTranslateApi {
public:
    explicit LibreTranslateApi(QString endpoint = QStringLiteral("https://libretranslate.com"));
    
    [[nodiscard]] QString endpoint() const { return endpoint_; }
    void setEndpoint(const QString& endpoint);
    
    QVector<QString> supportedLanguages();
    TranslationResult translate(const QString& text, const QString& sourceLang, const QString& targetLang);
    
private:
    QString endpoint_;
};

} // namespace ccos::api
