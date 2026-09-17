#pragma once
#include "core/Command.hpp"
#include "timeline/TimelineEditor.hpp"
#include "timeline/Track.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>

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

class DeleteClipCommand final : public ccos::core::Command {
public:
    DeleteClipCommand(Track& track, std::size_t clipIndex) : track_(track), clipIndex_(clipIndex) {
        if (clipIndex_ < track_.clips().size()) clip_ = track_.clips()[clipIndex_];
    }
    bool execute() override {
        if (executed_ || !clip_.has_value() || clipIndex_ >= track_.clips().size()) return false;
        track_.clips().erase(track_.clips().begin() + static_cast<std::ptrdiff_t>(clipIndex_));
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_ || !clip_.has_value()) return;
        const auto index = std::min(clipIndex_, track_.clips().size());
        track_.clips().insert(track_.clips().begin() + static_cast<std::ptrdiff_t>(index), *clip_);
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Delete Clip"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    std::optional<Clip> clip_;
    bool executed_ = false;
};

class SplitClipCommand final : public ccos::core::Command {
public:
    SplitClipCommand(Track& track, std::size_t clipIndex, ccos::core::Time timelineOffset)
        : track_(track), clipIndex_(clipIndex), timelineOffset_(timelineOffset) {}
    bool execute() override {
        if (executed_ || clipIndex_ >= track_.clips().size()) return false;
        original_ = track_.clips()[clipIndex_];
        if (!TimelineEditor::splitClip(track_, clipIndex_, timelineOffset_)) {
            original_.reset();
            return false;
        }
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_ || !original_.has_value() || clipIndex_ >= track_.clips().size()) return;
        track_.clips()[clipIndex_] = *original_;
        if (clipIndex_ + 1 < track_.clips().size()) {
            track_.clips().erase(track_.clips().begin() + static_cast<std::ptrdiff_t>(clipIndex_ + 1));
        }
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Split Clip"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    ccos::core::Time timelineOffset_{};
    std::optional<Clip> original_;
    bool executed_ = false;
};

}
