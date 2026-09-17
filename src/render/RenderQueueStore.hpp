#pragma once

#include <QVector>
#include <QString>

namespace ccos::render {

struct PersistedRenderJob {
    QString id;
    QString output;
    QString preset;
    QString state = QStringLiteral("queued");
    double progress = 0.0;
    QString error;
};

class RenderQueueStore final {
public:
    [[nodiscard]] static bool save(const QString& path,
                                   const QVector<PersistedRenderJob>& jobs,
                                   QString* error = nullptr);
    [[nodiscard]] static bool load(const QString& path,
                                   QVector<PersistedRenderJob>* jobs,
                                   QString* error = nullptr);
};

} // namespace ccos::render
