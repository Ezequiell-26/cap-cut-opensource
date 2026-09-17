#pragma once

#include "ai/AIProvider.hpp"

#include <memory>

namespace ccos::ai {

// Thin local-provider profile for llama.cpp's llama-server. llama-server
// exposes an OpenAI-compatible HTTP API, so this class reuses the hardened
// OpenAI-compatible transport already used by CCOS.
class LlamaCppProvider final : public AIProvider {
public:
    explicit LlamaCppProvider(QString endpoint = QStringLiteral("http://127.0.0.1:8080/v1"),
                              QString apiKey = QStringLiteral(""));

    [[nodiscard]] QString id() const override { return QStringLiteral("llama-cpp"); }
    [[nodiscard]] QString name() const override { return QStringLiteral("llama.cpp local server"); }

    AIResponse execute(const AIRequest& request) override;

private:
    std::unique_ptr<class OpenAICompatibleProvider> delegate_;
};

} // namespace ccos::ai
