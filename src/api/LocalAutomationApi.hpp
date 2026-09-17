#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <memory>
#include <thread>

namespace ccos::core {
class JobSystem;
}

namespace ccos::api {

/**
 * Optional loopback-only HTTP API for automation agents and external tools.
 * It exposes diagnostics and job control, not direct project mutation.
 */
class LocalAutomationApi final : public QObject {
    Q_OBJECT

public:
    explicit LocalAutomationApi(ccos::core::JobSystem* jobSystem,
                                quint16 port = 47999,
                                QString bearerToken = {},
                                QObject* parent = nullptr);
    ~LocalAutomationApi() override;

    LocalAutomationApi(const LocalAutomationApi&) = delete;
    LocalAutomationApi& operator=(const LocalAutomationApi&) = delete;

    [[nodiscard]] bool start();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }
    [[nodiscard]] quint16 port() const noexcept { return port_; }

Q_SIGNALS:
    void started(quint16 port);
    void stopped();
    void errorOccurred(const QString& message);

private:
    struct ServerHolder;
    void runServer();

    ccos::core::JobSystem* jobSystem_ = nullptr;
    quint16 port_ = 47999;
    QString bearerToken_;
    std::unique_ptr<ServerHolder> server_;
    std::thread serverThread_;
    std::atomic_bool running_{false};
    std::atomic_bool stopRequested_{false};
};

} // namespace ccos::api
