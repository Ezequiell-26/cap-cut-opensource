#pragma once
#include <QUuid>
#include <string>

namespace ccos::core {
class Uuid {
public:
    Uuid();
    explicit Uuid(const std::string& value);
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] bool isNull() const noexcept;
    [[nodiscard]] const QUuid& qt() const noexcept { return value_; }
    friend bool operator==(const Uuid&, const Uuid&) = default;
private:
    QUuid value_;
};
}
