#include "ai/OpenAICompatibleProvider.hpp"
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace ccos::ai {

OpenAICompatibleProvider::OpenAICompatibleProvider(QString endpoint, QString apiKey)
    : endpoint_(std::move(endpoint)), apiKey_(std::move(apiKey)) {
    while (endpoint_.endsWith('/')) endpoint_.chop(1);
}

AIResponse OpenAICompatibleProvider::execute(const AIRequest& request) {
    AIResponse result;
    const QString model = request.options.value(QStringLiteral("model")).toString();
    if (model.isEmpty()) {
        result.error = QStringLiteral("AI request option 'model' is required");
        return result;
    }
    if (request.input.trimmed().isEmpty()) {
        result.error = QStringLiteral("AI request input is empty");
        return result;
    }

    QJsonArray messages;
    const QString system = request.options.value(QStringLiteral("system")).toString();
    if (!system.isEmpty()) messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")}, {QStringLiteral("content"), system}});
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")}, {QStringLiteral("content"), request.input}});

    QJsonObject body{{QStringLiteral("model"), model}, {QStringLiteral("messages"), messages}};
    if (request.options.contains(QStringLiteral("temperature"))) body.insert(QStringLiteral("temperature"), request.options.value(QStringLiteral("temperature")).toDouble());
    if (request.options.contains(QStringLiteral("max_tokens"))) body.insert(QStringLiteral("max_tokens"), request.options.value(QStringLiteral("max_tokens")).toInt());

    QNetworkRequest networkRequest(QUrl(endpoint_ + QStringLiteral("/chat/completions")));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!apiKey_.isEmpty()) networkRequest.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey_.toUtf8());

    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.post(networkRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(request.options.value(QStringLiteral("timeoutMs")).toInt(120000));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop, reply] { reply->abort(); loop.quit(); });
    loop.exec();

    if (timeout.isActive()) timeout.stop();
    const QByteArray payload = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        result.error = reply->errorString();
        reply->deleteLater();
        return result;
    }
    reply->deleteLater();

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.error = QStringLiteral("Invalid JSON response from AI provider");
        return result;
    }
    const auto root = doc.object();
    const auto choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        result.error = root.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString(QStringLiteral("AI provider returned no choices"));
        return result;
    }
    result.output = choices.first().toObject().value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString();
    result.ok = !result.output.isEmpty();
    if (!result.ok) result.error = QStringLiteral("AI provider returned empty content");
    return result;
}

} // namespace ccos::ai
