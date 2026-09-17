(() => {
  const bridge = globalThis.ccosWebBridge || {};
  bridge.version = '1';
  bridge.isBrowser = true;
  bridge.ready = true;
  bridge.ensureViewport = () => {
    if (!document.querySelector('meta[name="viewport"]')) {
      const meta = document.createElement('meta');
      meta.name = 'viewport';
      meta.content = 'width=device-width, initial-scale=1, viewport-fit=cover';
      document.head.appendChild(meta);
    }
  };
  bridge.ensureViewport();
  globalThis.ccosWebBridge = bridge;
})();
