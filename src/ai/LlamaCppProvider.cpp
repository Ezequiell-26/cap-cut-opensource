#include "ai/LlamaCppProvider.hpp"

#include "ai/OpenAICompatibleProvider.hpp"

namespace ccos::ai {

LlamaCppProvider::LlamaCppProvider(QString endpoint, QString apiKey)
    : delegate_(std::make_unique<OpenAICompatibleProvider>(std::move(endpoint), std::move(apiKey))) {}

AIResponse LlamaCppProvider::execute(const AIRequest& request) {
    if (!delegate_) return AIResponse{false, {}, QStringLiteral("llama.cpp provider is not initialized")};
    return delegate_->execute(request);
}

} // namespace ccos::ai
