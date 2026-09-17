#pragma once
#include <QStringList>
#include <QVariantMap>

namespace ccos::effects {
struct BuiltinEffect {
    QString id;
    QString name;
    QVariantMap defaults;
};

class BuiltinEffects {
public:
    static QStringList ids();
    static BuiltinEffect get(const QString& id);
    static QString ffmpegFilter(const QString& id, const QVariantMap& parameters = {});
};
}
