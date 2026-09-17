#pragma once
#include "core/Time.hpp"
#include "core/Uuid.hpp"
#include <QString>

namespace ccos::text {
struct TextStyle {
    QString family = QStringLiteral("Inter");
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
    [[nodiscard]] const TextStyle& style() const noexcept { return style_; }
    TextStyle& style() noexcept { return style_; }
private:
    ccos::core::Uuid id_;
    QString text_;
    ccos::core::Time start_;
    ccos::core::Time duration_ = ccos::core::Time::fromSeconds(5.0);
    TextStyle style_;
};
}
