#pragma once
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace ccos::plugins {
struct PluginDescriptor {
    QString id;
    QString name;
    QString version;
    QString apiVersion;
    QString library;
    QStringList capabilities;
    QJsonObject raw;
};

class PluginManager {
public:
    static QVector<PluginDescriptor> scan(const QStringList& directories, QStringList* diagnostics = nullptr);
    static bool isCompatible(const PluginDescriptor& descriptor, const QString& apiVersion = QStringLiteral("1"));
};
}
