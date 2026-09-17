#include "render/RenderQueueStore.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace ccos::render {

bool RenderQueueStore::save(const QString& path, const QVector<PersistedRenderJob>& jobs, QString* error) {
    QJsonArray array;
    for (const auto& job : jobs) {
        array.append(QJsonObject{
            {QStringLiteral("id"), job.id},
            {QStringLiteral("output"), job.output},
            {QStringLiteral("preset"), job.preset},
            {QStringLiteral("state"), job.state},
            {QStringLiteral("progress"), job.progress},
            {QStringLiteral("error"), job.error}
        });
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }

    const QJsonDocument document(QJsonObject{
        {QStringLiteral("version"), 1},
        {QStringLiteral("jobs"), array}
    });
    const QByteArray payload = document.toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool RenderQueueStore::load(const QString& path, QVector<PersistedRenderJob>* jobs, QString* error) {
    if (!jobs) {
        if (error) *error = QStringLiteral("jobs output is required");
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = parseError.errorString();
        return false;
    }

    const QJsonValue version = document.object().value(QStringLiteral("version"));
    if (!version.isDouble() || version.toInt() != 1) {
        if (error) *error = QStringLiteral("Unsupported render queue schema version");
        return false;
    }

    QVector<PersistedRenderJob> parsed;
    const QJsonArray array = document.object().value(QStringLiteral("jobs")).toArray();
    parsed.reserve(array.size());
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        PersistedRenderJob job;
        job.id = object.value(QStringLiteral("id")).toString();
        job.output = object.value(QStringLiteral("output")).toString();
        job.preset = object.value(QStringLiteral("preset")).toString();
        job.state = object.value(QStringLiteral("state")).toString(QStringLiteral("queued"));
        job.progress = object.value(QStringLiteral("progress")).toDouble(0.0);
        job.error = object.value(QStringLiteral("error")).toString();
        if (job.id.isEmpty() || job.output.isEmpty() || job.progress < 0.0 || job.progress > 1.0) {
            if (error) *error = QStringLiteral("Invalid render queue job entry");
            return false;
        }
        parsed.append(std::move(job));
    }

    *jobs = std::move(parsed);
    return true;
}

} // namespace ccos::render
