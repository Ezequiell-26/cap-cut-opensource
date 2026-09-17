#pragma once

#include <QString>

#include <functional>

namespace ccos::web {

class BrowserStorage final {
public:
    using Callback = std::function<void(bool ok, const QString& error)>;

    static void initialize(Callback callback = {});
    static void sync(Callback callback = {});
    [[nodiscard]] static bool available() noexcept;
};

} // namespace ccos::web
