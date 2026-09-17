#pragma once
#include "ai/AIProvider.hpp"

namespace ccos::ai {

class OpenAICompatibleProvider final : public AIProvider {
public:
    explicit OpenAICompatibleProvider(QString endpoint = QStringLiteral("http://localhost:11434/v1"),
                                      QString apiKey = QStringLiteral(""));
    [[nodiscard]] QString id() const override { return QStringLiteral("openai-compatible"); }
    [[nodiscard]] QString name() const override { return QStringLiteral("OpenAI-compatible local provider"); }
    AIResponse execute(const AIRequest& request) override;

private:
    QString endpoint_;
    QString apiKey_;
};

} // namespace ccos::ai
