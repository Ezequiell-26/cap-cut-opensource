#pragma once
#include "core/Time.hpp"
#include "core/Uuid.hpp"
#include <QString>

namespace ccos::text {
struct TextStyle {
    QString family = QStringLiteral("DejaVu Sans");
    double size = 64.0;
    QString color = QStringLiteral("#FFFFFF");
    bool bold = false;
    bool italic = false;
    double opacity = 1.0;
};

class TextLayer {
public:
    TextLayer() = default;
    explicit TextLayer(QString text) : text_(std::move(text)) {}
    [[nodiscard]] const ccos::core::Uuid& id() const noexcept { return id_; }
    [[nodiscard]] const QString& text() const noexcept { return text_; }
    void setText(QString text) { text_ = std::move(text); }
    [[nodiscard]] ccos::core::Time start() const noexcept { return start_; }
    [[nodiscard]] ccos::core::Time duration() const noexcept { return duration_; }
    void setStart(ccos::core::Time value) noexcept { start_ = value; }
    void setDuration(ccos::core::Time value) noexcept { duration_ = value; }
    [[nodiscard]] double x() const noexcept { return x_; }
    [[nodiscard]] double y() const noexcept { return y_; }
    void setPosition(double x, double y) noexcept { x_ = x; y_ = y; }
    [[nodiscard]] const TextStyle& style() const noexcept { return style_; }
    TextStyle& style() noexcept { return style_; }
private:
    ccos::core::Uuid id_;
    QString text_;
    ccos::core::Time start_;
    ccos::core::Time duration_ = ccos::core::Time::fromSeconds(5.0);
    double x_ = 0.5;
    double y_ = 0.85;
    TextStyle style_;
};
}
