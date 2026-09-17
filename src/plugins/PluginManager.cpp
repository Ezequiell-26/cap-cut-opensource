#include "plugins/PluginManager.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>

namespace ccos::plugins {

QVector<PluginDescriptor> PluginManager::scan(const QStringList& directories, QStringList* diagnostics) {
    QVector<PluginDescriptor> result;
    for (const auto& directory : directories) {
        QDir dir(directory);
        if (!dir.exists()) { if (diagnostics) diagnostics->append(QStringLiteral("Plugin directory missing: %1").arg(directory)); continue; }
        const auto manifests = dir.entryInfoList({QStringLiteral("*.ccosplugin.json")}, QDir::Files | QDir::Readable);
        for (const auto& fileInfo : manifests) {
            QFile file(fileInfo.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly)) { if (diagnostics) diagnostics->append(QStringLiteral("Cannot read %1").arg(fileInfo.fileName())); continue; }
            QJsonParseError parse{};
            const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
            if (parse.error != QJsonParseError::NoError || !doc.isObject()) { if (diagnostics) diagnostics->append(QStringLiteral("Invalid manifest: %1").arg(fileInfo.fileName())); continue; }
            const auto o = doc.object();
            PluginDescriptor d;
            d.id = o.value(QStringLiteral("id")).toString();
            d.name = o.value(QStringLiteral("name")).toString();
            d.version = o.value(QStringLiteral("version")).toString();
            d.apiVersion = o.value(QStringLiteral("apiVersion")).toString();
            d.library = o.value(QStringLiteral("library")).toString();
            for (const auto& v : o.value(QStringLiteral("capabilities")).toArray()) d.capabilities.append(v.toString());
            d.raw = o;
            if (!d.id.isEmpty() && !d.name.isEmpty()) result.append(std::move(d));
        }
    }
    return result;
}

bool PluginManager::isCompatible(const PluginDescriptor& descriptor, const QString& apiVersion) {
    return descriptor.apiVersion == apiVersion && !descriptor.library.isEmpty();
}
}
