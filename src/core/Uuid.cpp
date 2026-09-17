#include "core/Uuid.hpp"

namespace ccos::core {
Uuid::Uuid() : value_(QUuid::createUuid()) {}
Uuid::Uuid(const std::string& value) : value_(QUuid::fromString(QString::fromStdString(value))) {}
std::string Uuid::toString() const { return value_.toString(QUuid::WithoutBraces).toStdString(); }
bool Uuid::isNull() const noexcept { return value_.isNull(); }
}
