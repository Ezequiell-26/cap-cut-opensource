#include "api/LibreTranslateApi.hpp"
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace ccos::api {

LibreTranslateApi::LibreTranslateApi(QString endpoint) : endpoint_(std::move(endpoint)) {
    while (endpoint_.endsWith('/')) endpoint_.chop(1);
}

void LibreTranslateApi::setEndpoint(const QString& endpoint) {
    endpoint_ = endpoint;
    while (endpoint_.endsWith('/')) endpoint_.chop(1);
}

QVector<QString> LibreTranslateApi::supportedLanguages() {
    QVector<QString> languages;
    
    QUrl url(endpoint_ + QStringLiteral("/languages"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(15000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply] { reply->abort(); loop.quit(); });
    loop.exec();
    
    if (timeout.isActive()) timeout.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return languages;
    }
    
    const QByteArray payload = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return languages;
    }
    
    const auto arr = doc.array();
    for (const auto& item : arr) {
        const auto obj = item.toObject();
        const QString code = obj.value(QStringLiteral("code")).toString();
        if (!code.isEmpty()) {
            languages.append(code);
        }
    }
    
    return languages;
}

TranslationResult LibreTranslateApi::translate(const QString& text, const QString& sourceLang, const QString& targetLang) {
    TranslationResult result;
    result.sourceLanguage = sourceLang;
    result.targetLanguage = targetLang;
    result.originalText = text;
    result.provider = QStringLiteral("LibreTranslate");
    
    if (text.trimmed().isEmpty()) {
        result.translatedText = QString();
        return result;
    }
    
    QUrl url(endpoint_ + QStringLiteral("/translate"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    
    QJsonObject body{
        {QStringLiteral("q"), text},
        {QStringLiteral("source"), sourceLang},
        {QStringLiteral("target"), targetLang},
        {QStringLiteral("format"), QStringLiteral("text")}
    };
    
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(30000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply] { reply->abort(); loop.quit(); });
    loop.exec();
    
    if (timeout.isActive()) timeout.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return result;
    }
    
    const QByteArray payload = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return result;
    }
    
    const auto root = doc.object();
    result.translatedText = root.value(QStringLiteral("translatedText")).toString();
    
    return result;
}

} // namespace ccos::api
