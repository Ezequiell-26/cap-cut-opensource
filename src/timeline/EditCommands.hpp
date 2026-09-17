#pragma once
#include "core/Command.hpp"
#include "timeline/Track.hpp"

namespace ccos::timeline {
class AddClipCommand final : public ccos::core::Command {
public:
    AddClipCommand(Track& track, Clip clip) : track_(track), clip_(std::move(clip)) {}
    bool execute() override { if (executed_) return true; track_.addClip(clip_); executed_ = true; return true; }
    void undo() override { if (!executed_ || track_.clips().empty()) return; track_.clips().pop_back(); executed_ = false; }
    QString name() const override { return QStringLiteral("Add Clip"); }
private:
    Track& track_;
    Clip clip_;
    bool executed_ = false;
};
}
