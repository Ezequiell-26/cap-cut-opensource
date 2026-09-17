#include "api/LocalAutomationApi.hpp"
#include "core/JobSystem.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include <httplib.h>

#include <utility>

namespace ccos::api {

struct LocalAutomationApi::ServerHolder {
    httplib::Server server;
};

namespace {

QByteArray jsonResponse(const QJsonObject& object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QJsonObject jobToJson(const ccos::core::JobStatus& status) {
    QJsonObject object;
    object.insert(QStringLiteral("id"), status.id);
    object.insert(QStringLiteral("type"), status.typeToString());
    object.insert(QStringLiteral("name"), status.name);
    object.insert(QStringLiteral("state"), status.stateToString());
    object.insert(QStringLiteral("priority"), status.priority);
    object.insert(QStringLiteral("progress"), status.progress);
    object.insert(QStringLiteral("createdAt"), status.createdAt.toString(Qt::ISODateWithMs));
    object.insert(QStringLiteral("startedAt"), status.startedAt.toString(Qt::ISODateWithMs));
    object.insert(QStringLiteral("completedAt"), status.completedAt.toString(Qt::ISODateWithMs));
    object.insert(QStringLiteral("retryCount"), status.retryCount);
    object.insert(QStringLiteral("maxRetries"), status.maxRetries);
    object.insert(QStringLiteral("cancelable"), status.cancelable);
    object.insert(QStringLiteral("errorCode"), status.error.code);
    object.insert(QStringLiteral("error"), status.error.message);
    return object;
}

bool authorized(const httplib::Request& request, const QString& token) {
    if (token.isEmpty()) return true;
    const std::string expected = (QStringLiteral("Bearer ") + token).toStdString();
    return request.get_header_value("Authorization") == expected;
}

void unauthorized(httplib::Response& response) {
    response.status = 401;
    response.set_header("WWW-Authenticate", "Bearer");
    response.set_content("{\"error\":\"unauthorized\"}", "application/json");
}

} // namespace

LocalAutomationApi::LocalAutomationApi(ccos::core::JobSystem* jobSystem,
                                       quint16 port,
                                       QString bearerToken,
                                       QObject* parent)
    : QObject(parent)
    , jobSystem_(jobSystem)
    , port_(port)
    , bearerToken_(std::move(bearerToken)) {}

LocalAutomationApi::~LocalAutomationApi() {
    stop();
}

bool LocalAutomationApi::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return false;

    stopRequested_.store(false, std::memory_order_release);
    server_ = std::make_unique<ServerHolder>();
    auto* holder = server_.get();

    holder->server.set_keep_alive_max_count(100);

    holder->server.Get("/api/v1/health", [this](const httplib::Request& request, httplib::Response& response) {
        if (!authorized(request, bearerToken_)) {
            unauthorized(response);
            return;
        }
        response.set_content("{\"status\":\"ok\",\"service\":\"ccos-local-automation\",\"version\":1}",
                             "application/json");
    });

    holder->server.Get("/api/v1/jobs", [this](const httplib::Request& request, httplib::Response& response) {
        if (!authorized(request, bearerToken_)) {
            unauthorized(response);
            return;
        }

        QJsonArray jobs;
        if (jobSystem_) {
            for (const auto& job : jobSystem_->getAllJobs()) jobs.append(jobToJson(job));
        }
        response.set_content(jsonResponse(QJsonObject{{QStringLiteral("jobs"), jobs}}).toStdString(),
                             "application/json");
    });

    holder->server.Get(R"(/api/v1/jobs/([^/]+))", [this](const httplib::Request& request, httplib::Response& response) {
        if (!authorized(request, bearerToken_)) {
            unauthorized(response);
            return;
        }
        if (!jobSystem_) {
            response.status = 503;
            response.set_content("{\"error\":\"job system unavailable\"}", "application/json");
            return;
        }

        const QString jobId = QString::fromStdString(request.matches[1]);
        const auto status = jobSystem_->getJobStatus(jobId);
        if (!status.has_value()) {
            response.status = 404;
            response.set_content("{\"error\":\"job not found\"}", "application/json");
            return;
        }
        response.set_content(jsonResponse(jobToJson(*status)).toStdString(), "application/json");
    });

    holder->server.Post(R"(/api/v1/jobs/([^/]+)/cancel)", [this](const httplib::Request& request, httplib::Response& response) {
        if (!authorized(request, bearerToken_)) {
            unauthorized(response);
            return;
        }
        if (!jobSystem_) {
            response.status = 503;
            response.set_content("{\"error\":\"job system unavailable\"}", "application/json");
            return;
        }

        const QString jobId = QString::fromStdString(request.matches[1]);
        const bool cancelled = jobSystem_->cancelJob(jobId);
        response.status = cancelled ? 200 : 409;
        response.set_content(jsonResponse(QJsonObject{
            {QStringLiteral("jobId"), jobId},
            {QStringLiteral("cancelled"), cancelled}
        }).toStdString(), "application/json");
    });

    serverThread_ = std::thread(&LocalAutomationApi::runServer, this);
    return true;
}

void LocalAutomationApi::stop() {
    stopRequested_.store(true, std::memory_order_release);

    if (server_) server_->server.stop();
    if (serverThread_.joinable()) serverThread_.join();
    server_.reset();

    if (running_.exchange(false, std::memory_order_acq_rel)) emit stopped();
}

void LocalAutomationApi::runServer() {
    auto* holder = server_.get();
    if (holder == nullptr) {
        running_.store(false, std::memory_order_release);
        return;
    }

    const int socket = holder->server.bind_to_port("127.0.0.1", static_cast<int>(port_));
    if (socket < 0) {
        running_.store(false, std::memory_order_release);
        if (!stopRequested_.load(std::memory_order_acquire)) {
            QMetaObject::invokeMethod(this, [this]() {
                emit errorOccurred(QStringLiteral("Could not bind local automation API on 127.0.0.1:%1")
                                       .arg(port_));
            }, Qt::QueuedConnection);
        }
        return;
    }

    if (!stopRequested_.load(std::memory_order_acquire)) {
        QMetaObject::invokeMethod(this, [this]() { emit started(port_); }, Qt::QueuedConnection);
    }

    holder->server.listen_after_bind();
}

} // namespace ccos::api
