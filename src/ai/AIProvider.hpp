#pragma once
#include <QString>
#include <QVariantMap>

namespace ccos::ai {
struct AIRequest { QString task; QString input; QVariantMap options; };
struct AIResponse { bool ok = false; QString output; QString error; };

class AIProvider {
public:
    virtual ~AIProvider() = default;
    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual QString name() const = 0;
    virtual AIResponse execute(const AIRequest& request) = 0;
};
}
