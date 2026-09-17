#pragma once

#include "api/CreativeMediaApi.hpp"

#include <QObject>
#include <QString>
#include <functional>

class QJsonDocument;
class QNetworkAccessManager;

namespace ccos::api {

// NASA Astronomy Picture of the Day adapter.
// DEMO_KEY is used by default; callers may provide their own key.
class NasaMediaApi final : public QObject {
    Q_OBJECT

public:
    explicit NasaMediaApi(QObject* parent = nullptr);
    ~NasaMediaApi() override;

    void setApiKey(const QString& apiKey);
    [[nodiscard]] QString apiKey() const { return apiKey_; }

    void fetchApod(const QString& date,
                   std::function<void(const CreativeMediaItem&)> callback);

    [[nodiscard]] static CreativeMediaItem parseApod(const QJsonDocument& document);

Q_SIGNALS:
    void mediaReady(const CreativeMediaItem& item);
    void errorOccurred(const QString& message);

private:
    QString apiKey_ = QStringLiteral("DEMO_KEY");
    QNetworkAccessManager* networkManager_ = nullptr;
};

} // namespace ccos::api
