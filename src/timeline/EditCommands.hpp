#pragma once
#include "core/Command.hpp"
#include "timeline/TimelineEditor.hpp"
#include "timeline/Track.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

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

class TrimClipCommand final : public ccos::core::Command {
public:
    TrimClipCommand(Track& track, std::size_t clipIndex, ccos::core::Time newIn, ccos::core::Time newOut)
        : track_(track), clipIndex_(clipIndex), newIn_(newIn), newOut_(newOut) {}
    bool execute() override {
        if (executed_ || clipIndex_ >= track_.clips().size()) return false;
        if (!original_.has_value()) original_ = track_.clips()[clipIndex_];
        if (!TimelineEditor::trimClip(track_, clipIndex_, newIn_, newOut_)) return false;
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_ || !original_.has_value() || clipIndex_ >= track_.clips().size()) return;
        track_.clips()[clipIndex_] = *original_;
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Trim Clip"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    ccos::core::Time newIn_{};
    ccos::core::Time newOut_{};
    std::optional<Clip> original_;
    bool executed_ = false;
};

class MoveClipCommand final : public ccos::core::Command {
public:
    MoveClipCommand(Track& track, std::size_t clipIndex, ccos::core::Time newStart)
        : track_(track), clipIndex_(clipIndex), newStart_(newStart) {}
    bool execute() override {
        if (executed_ || clipIndex_ >= track_.clips().size() || newStart_ < ccos::core::Time{}) return false;
        if (!originalStart_.has_value()) originalStart_ = track_.clips()[clipIndex_].start();
        track_.clips()[clipIndex_].setStart(newStart_);
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_ || !originalStart_.has_value() || clipIndex_ >= track_.clips().size()) return;
        track_.clips()[clipIndex_].setStart(*originalStart_);
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Move Clip"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    ccos::core::Time newStart_{};
    std::optional<ccos::core::Time> originalStart_;
    bool executed_ = false;
};

class RippleDeleteClipCommand final : public ccos::core::Command {
public:
    RippleDeleteClipCommand(Track& track, std::size_t clipIndex) : track_(track), clipIndex_(clipIndex) {}
    bool execute() override {
        if (executed_ || clipIndex_ >= track_.clips().size()) return false;
        original_ = track_.clips();
        if (!TimelineEditor::rippleDelete(track_, clipIndex_)) {
            original_.clear();
            return false;
        }
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_) return;
        track_.clips() = original_;
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Ripple Delete Clip"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    std::vector<Clip> original_;
    bool executed_ = false;
};

class SetClipSpeedCommand final : public ccos::core::Command {
public:
    SetClipSpeedCommand(Track& track, std::size_t clipIndex, double speed)
        : track_(track), clipIndex_(clipIndex), newSpeed_(speed) {}
    bool execute() override {
        if (executed_ || clipIndex_ >= track_.clips().size() || newSpeed_ <= 0.0) return false;
        if (!originalSpeed_.has_value()) originalSpeed_ = track_.clips()[clipIndex_].speed();
        track_.clips()[clipIndex_].setSpeed(newSpeed_);
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_ || !originalSpeed_.has_value() || clipIndex_ >= track_.clips().size()) return;
        track_.clips()[clipIndex_].setSpeed(*originalSpeed_);
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Set Clip Speed"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    double newSpeed_ = 1.0;
    std::optional<double> originalSpeed_;
    bool executed_ = false;
};

class AddEffectCommand final : public ccos::core::Command {
public:
    AddEffectCommand(Track& track, std::size_t clipIndex, QString effectId)
        : track_(track), clipIndex_(clipIndex), effectId_(std::move(effectId)) {}
    bool execute() override {
        if (executed_ || clipIndex_ >= track_.clips().size() || effectId_.isEmpty()) return false;
        if (track_.clips()[clipIndex_].effects().contains(effectId_)) return false;
        track_.clips()[clipIndex_].addEffect(effectId_);
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_ || clipIndex_ >= track_.clips().size()) return;
        track_.clips()[clipIndex_].removeEffect(effectId_);
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Add Effect"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    QString effectId_;
    bool executed_ = false;
};

class SetTransitionCommand final : public ccos::core::Command {
public:
    SetTransitionCommand(Track& track, std::size_t clipIndex, QString transitionId, qint64 durationMs)
        : track_(track), clipIndex_(clipIndex), transitionId_(std::move(transitionId)), durationMs_(durationMs) {}
    bool execute() override {
        if (executed_ || clipIndex_ >= track_.clips().size() || transitionId_.isEmpty() || durationMs_ < 0) return false;
        if (!original_.has_value()) original_ = qMakePair(track_.clips()[clipIndex_].transitionInId(), track_.clips()[clipIndex_].transitionInDurationMs());
        track_.clips()[clipIndex_].setTransitionIn(transitionId_, durationMs_);
        executed_ = true;
        return true;
    }
    void undo() override {
        if (!executed_ || !original_.has_value() || clipIndex_ >= track_.clips().size()) return;
        track_.clips()[clipIndex_].setTransitionIn(original_->first, original_->second);
        executed_ = false;
    }
    QString name() const override { return QStringLiteral("Set Transition"); }
private:
    Track& track_;
    std::size_t clipIndex_ = 0;
    QString transitionId_;
    qint64 durationMs_ = 0;
    std::optional<QPair<QString, qint64>> original_;
    bool executed_ = false;
};

} // namespace ccos::timeline
