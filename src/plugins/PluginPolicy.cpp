#include "plugins/PluginPolicy.hpp"

namespace ccos::plugins {

QString PluginPolicy::capabilityName(PluginCapability capability) {
    switch (capability) {
        case PluginCapability::ReadProject: return QStringLiteral("read_project");
        case PluginCapability::WriteProject: return QStringLiteral("write_project");
        case PluginCapability::ReadMedia: return QStringLiteral("read_media");
        case PluginCapability::WriteMedia: return QStringLiteral("write_media");
        case PluginCapability::Network: return QStringLiteral("network");
        case PluginCapability::SpawnProcess: return QStringLiteral("spawn_process");
        case PluginCapability::AccessDevice: return QStringLiteral("access_device");
        case PluginCapability::UiExtension: return QStringLiteral("ui_extension");
    }
    return QStringLiteral("unknown");
}

bool PluginPolicy::isHighRisk(PluginCapability capability) noexcept {
    return capability == PluginCapability::WriteProject ||
           capability == PluginCapability::WriteMedia ||
           capability == PluginCapability::Network ||
           capability == PluginCapability::SpawnProcess ||
           capability == PluginCapability::AccessDevice;
}

bool PluginPolicy::validateRequested(const QSet<PluginCapability>& requested,
                                     const QSet<PluginCapability>& granted,
                                     QString* error) {
    for (const PluginCapability capability : requested) {
        if (granted.contains(capability)) continue;
        if (error) {
            *error = QStringLiteral("Plugin capability not granted: %1")
                .arg(capabilityName(capability));
        }
        return false;
    }
    return true;
}

} // namespace ccos::plugins
