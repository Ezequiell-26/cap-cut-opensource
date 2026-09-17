#pragma once

#include <QSet>
#include <QString>

namespace ccos::plugins {

enum class PluginCapability {
    ReadProject,
    WriteProject,
    ReadMedia,
    WriteMedia,
    Network,
    SpawnProcess,
    AccessDevice,
    UiExtension
};

class PluginPolicy final {
public:
    [[nodiscard]] static QString capabilityName(PluginCapability capability);
    [[nodiscard]] static bool isHighRisk(PluginCapability capability) noexcept;
    [[nodiscard]] static bool validateRequested(const QSet<PluginCapability>& requested,
                                                const QSet<PluginCapability>& granted,
                                                QString* error = nullptr);
};

} // namespace ccos::plugins
