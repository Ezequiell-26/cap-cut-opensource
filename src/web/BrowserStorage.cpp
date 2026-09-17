#include "web/BrowserStorage.hpp"

#include <QCoreApplication>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <mutex>
#  include <unordered_map>

namespace {

using Callback = ccos::web::BrowserStorage::Callback;
std::mutex g_callbackMutex;
int g_nextCallbackId = 1;
std::unordered_map<int, Callback> g_callbacks;

extern "C" EMSCRIPTEN_KEEPALIVE void ccos_browser_storage_complete(int callbackId, int error) {
    Callback callback;
    {
        std::lock_guard lock(g_callbackMutex);
        const auto it = g_callbacks.find(callbackId);
        if (it == g_callbacks.end()) return;
        callback = std::move(it->second);
        g_callbacks.erase(it);
    }

    if (!callback) return;
    QCoreApplication::postEvent(
        QCoreApplication::instance(),
        new QEvent(static_cast<QEvent::Type>(QEvent::User + 8123)));

    callback(error == 0,
             error == 0 ? QString{}
                        : QStringLiteral("Browser persistent storage synchronization failed"));
}

EM_JS(void, ccos_browser_storage_start, (int callbackId, int populate), {
    try {
        if (!FS.analyzePath('/ccos').exists) {
            FS.mkdir('/ccos');
        }
        const mounted = FS.mounts && FS.mounts.some(function (mount) {
            return mount.mountpoint === '/ccos';
        });
        if (!mounted) {
            FS.mount(IDBFS, {}, '/ccos');
        }
        FS.syncfs(!!populate, function (err) {
            if (err) {
                ccall('ccos_browser_storage_complete', 'void', ['number', 'number'], [callbackId, 1]);
                return;
            }
            ccall('ccos_browser_storage_complete', 'void', ['number', 'number'], [callbackId, 0]);
        });
    } catch (e) {
        console.error('CCOS browser storage error', e);
        ccall('ccos_browser_storage_complete', 'void', ['number', 'number'], [callbackId, 1]);
    }
});

EM_JS(void, ccos_browser_storage_sync, (int callbackId), {
    try {
        FS.syncfs(false, function (err) {
            ccall('ccos_browser_storage_complete', 'void', ['number', 'number'], [callbackId, err ? 1 : 0]);
        });
    } catch (e) {
        console.error('CCOS browser storage sync error', e);
        ccall('ccos_browser_storage_complete', 'void', ['number', 'number'], [callbackId, 1]);
    }
});

int registerCallback(Callback callback) {
    if (!callback) return 0;
    std::lock_guard lock(g_callbackMutex);
    const int id = g_nextCallbackId++;
    g_callbacks.emplace(id, std::move(callback));
    return id;
}

} // namespace

namespace ccos::web {

void BrowserStorage::initialize(Callback callback) {
    const int id = registerCallback(std::move(callback));
    ccos_browser_storage_start(id, 1);
}

void BrowserStorage::sync(Callback callback) {
    const int id = registerCallback(std::move(callback));
    ccos_browser_storage_sync(id);
}

bool BrowserStorage::available() noexcept {
    return true;
}

} // namespace ccos::web

#else

namespace ccos::web {

void BrowserStorage::initialize(Callback callback) {
    if (callback) callback(true, {});
}

void BrowserStorage::sync(Callback callback) {
    if (callback) callback(true, {});
}

bool BrowserStorage::available() noexcept {
    return false;
}

} // namespace ccos::web

#endif
