#include "web/BrowserStorage.hpp"

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <mutex>
#  include <unordered_map>
#  include <utility>

namespace {
using Callback = ccos::web::BrowserStorage::Callback;
std::mutex g_callbackMutex;
int g_nextCallbackId = 1;
std::unordered_map<int, Callback> g_callbacks;

int registerCallback(Callback callback) {
    if (!callback) return 0;
    std::lock_guard lock(g_callbackMutex);
    const int id = g_nextCallbackId++;
    g_callbacks.emplace(id, std::move(callback));
    return id;
}

extern "C" EMSCRIPTEN_KEEPALIVE void ccos_browser_storage_complete(int callbackId, int error) {
    Callback callback;
    {
        std::lock_guard lock(g_callbackMutex);
        const auto it = g_callbacks.find(callbackId);
        if (it == g_callbacks.end()) return;
        callback = std::move(it->second);
        g_callbacks.erase(it);
    }
    if (callback) {
        callback(error == 0, error == 0 ? QString{} : QStringLiteral("IndexedDB browser storage synchronization failed"));
    }
}

EM_JS(void, ccos_browser_storage_initialize, (int callbackId), {
    const done = (error) => ccall('ccos_browser_storage_complete', 'void', ['number', 'number'], [callbackId, error ? 1 : 0]);
    try {
        if (!FS.analyzePath('/ccos').exists) FS.mkdir('/ccos');
        const isMounted = FS.mounts && FS.mounts.some((mount) => mount.mountpoint === '/ccos');
        if (!isMounted) FS.mount(IDBFS, {}, '/ccos');
        if (!globalThis.__ccosStorageHooksInstalled) {
            globalThis.__ccosStorageHooksInstalled = true;
            const sync = () => {
                try { FS.syncfs(false, () => {}); } catch (e) { console.warn('CCOS storage sync failed', e); }
            };
            document.addEventListener('visibilitychange', () => {
                if (document.visibilityState === 'hidden') sync();
            });
            window.addEventListener('pagehide', sync);
        }
        FS.syncfs(true, done);
    } catch (e) {
        console.error('CCOS browser storage initialization failed', e);
        done(e);
    }
});

EM_JS(void, ccos_browser_storage_sync, (int callbackId), {
    const done = (error) => ccall('ccos_browser_storage_complete', 'void', ['number', 'number'], [callbackId, error ? 1 : 0]);
    try { FS.syncfs(false, done); }
    catch (e) {
        console.error('CCOS browser storage sync failed', e);
        done(e);
    }
});
}

namespace ccos::web {

void BrowserStorage::initialize(Callback callback) {
    const int id = registerCallback(std::move(callback));
    ccos_browser_storage_initialize(id);
}

void BrowserStorage::sync(Callback callback) {
    const int id = registerCallback(std::move(callback));
    ccos_browser_storage_sync(id);
}

bool BrowserStorage::available() noexcept { return true; }

} // namespace ccos::web

#else

namespace ccos::web {
void BrowserStorage::initialize(Callback callback) { if (callback) callback(true, {}); }
void BrowserStorage::sync(Callback callback) { if (callback) callback(true, {}); }
bool BrowserStorage::available() noexcept { return false; }
} // namespace ccos::web

#endif
