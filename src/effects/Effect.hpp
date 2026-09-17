#pragma once
#include <QString>
#include <QVariantMap>

namespace ccos::effects {
class Effect {
public:
    virtual ~Effect() = default;
    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual QString displayName() const = 0;
    [[nodiscard]] virtual QVariantMap parameters() const = 0;
    virtual void setParameter(const QString& name, const QVariant& value) = 0;
};
}
