#include "ui/MainWindow.hpp"

#include "effects/BuiltinEffects.hpp"
#include "timeline/EditCommands.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QInputDialog>
#include <QMenu>
#include <QMetaObject>
#include <QObject>
#include <QPair>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QToolBar>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace ccos::ui {
namespace {

class AdvancedTimelineEventFilter final : public QObject {
public:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event && (event->type() == QEvent::Show || event->type() == QEvent::WindowActivate)) {
            if (auto* window = qobject_cast<MainWindow*>(watched)) {
                if (!window->property("ccos.advancedTimelineInstalled").toBool()) {
                    QMetaObject::invokeMethod(window, "installAdvancedTimelineUi", Qt::QueuedConnection);
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

void initializeAdvancedTimelineUi() {
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    static auto* filter = new AdvancedTimelineEventFilter();
    app->installEventFilter(filter);
    for (QWidget* widget : app->topLevelWidgets()) {
        if (auto* window = qobject_cast<MainWindow*>(widget)) {
            QMetaObject::invokeMethod(window, "installAdvancedTimelineUi", Qt::QueuedConnection);
        }
    }
}

} // namespace

void MainWindow::installAdvancedTimelineUi() {
    if (property("ccos.advancedTimelineInstalled").toBool()) return;
    setProperty("ccos.advancedTimelineInstalled", true);

    timeline_->setSelectionMode(QAbstractItemView::SingleSelection);
    timeline_->setContextMenuPolicy(Qt::CustomContextMenu);

    auto findSelection = [this]() -> std::optional<std::pair<std::size_t, std::size_t>> {
        auto* item = timeline_ ? timeline_->currentItem() : nullptr;
        if (!item || !item->parent()) return std::nullopt;
        const QString id = item->toolTip(0).trimmed();
        if (id.isEmpty()) return std::nullopt;
        const auto& tracks = project_.timeline().tracks();
        for (std::size_t ti = 0; ti < tracks.size(); ++ti) {
            const auto& clips = tracks[ti].clips();
            for (std::size_t ci = 0; ci < clips.size(); ++ci) {
                if (QString::fromStdString(clips[ci].id().toString()) == id) {
                    return std::make_pair(ti, ci);
                }
            }
        }
        return std::nullopt;
    };

    connect(timeline_, &QTreeWidget::itemSelectionChanged, this, [this, findSelection] {
        const auto selection = findSelection();
        if (!selection.has_value()) return;
        const auto [trackIndex, clipIndex] = *selection;
        const auto& tracks = project_.timeline().tracks();
        if (trackIndex >= tracks.size() || clipIndex >= tracks[trackIndex].clips().size()) return;
        const auto& clip = tracks[trackIndex].clips()[clipIndex];
        for (const auto& asset : project_.assets()) {
            if (asset.id() == clip.assetId()) {
                previewLabel_->setText(asset.name());
                loadPreviewSource(asset.path());
                player_->setPosition(static_cast<qint64>(clip.sourceIn().seconds() * 1000.0));
                break;
            }
        }
    });

    auto createAction = [this](QMenu* menu, const QString& text, const QKeySequence& shortcut, const std::function<void()>& handler) {
        auto* action = menu->addAction(text);
        if (!shortcut.isEmpty()) action->setShortcut(shortcut);
        connect(action, &QAction::triggered, this, [handler] { handler(); });
        return action;
    };

    auto* timelineMenu = menuBar()->addMenu(QStringLiteral("Timeline"));
    timelineMenu->addSection(QStringLiteral("Edit Clip"));
    createAction(timelineMenu, QStringLiteral("Split at Playhead"), QKeySequence(QStringLiteral("Ctrl+K")), [this] { splitSelectedClip(); });
    createAction(timelineMenu, QStringLiteral("Delete Clip"), QKeySequence::Delete, [this] { deleteSelectedClip(); });
    createAction(timelineMenu, QStringLiteral("Ripple Delete Clip"), QKeySequence(QStringLiteral("Ctrl+Shift+Delete")), [this] { rippleDeleteSelectedClip(); });
    timelineMenu->addSeparator();
    createAction(timelineMenu, QStringLiteral("Trim Start to Playhead"), QKeySequence(QStringLiteral("Shift+[")), [this] { trimSelectedClipStart(); });
    createAction(timelineMenu, QStringLiteral("Trim End to Playhead"), QKeySequence(QStringLiteral("Shift+]")), [this] { trimSelectedClipEnd(); });
    createAction(timelineMenu, QStringLiteral("Nudge Left 1/30s"), QKeySequence(QStringLiteral("Alt+Left")), [this] { nudgeSelectedClipLeft(); });
    createAction(timelineMenu, QStringLiteral("Nudge Right 1/30s"), QKeySequence(QStringLiteral("Alt+Right")), [this] { nudgeSelectedClipRight(); });
    timelineMenu->addSeparator();
    createAction(timelineMenu, QStringLiteral("Set Speed…"), {}, [this] { setSelectedClipSpeed(); });
    createAction(timelineMenu, QStringLiteral("Add Effect…"), {}, [this] { addEffectToSelectedClip(); });
    createAction(timelineMenu, QStringLiteral("Set Transition…"), {}, [this] { setTransitionOnSelectedClip(); });

    auto* toolbar = addToolBar(QStringLiteral("Timeline Editing"));
    toolbar->setMovable(false);
    toolbar->addAction(QStringLiteral("Split"), this, &MainWindow::splitSelectedClip);
    toolbar->addAction(QStringLiteral("Delete"), this, &MainWindow::deleteSelectedClip);
    toolbar->addAction(QStringLiteral("Ripple Delete"), this, &MainWindow::rippleDeleteSelectedClip);
    toolbar->addSeparator();
    toolbar->addAction(QStringLiteral("Trim In"), this, &MainWindow::trimSelectedClipStart);
    toolbar->addAction(QStringLiteral("Trim Out"), this, &MainWindow::trimSelectedClipEnd);
    toolbar->addSeparator();
    toolbar->addAction(QStringLiteral("Speed"), this, &MainWindow::setSelectedClipSpeed);
    toolbar->addAction(QStringLiteral("Effect"), this, &MainWindow::addEffectToSelectedClip);
    toolbar->addAction(QStringLiteral("Transition"), this, &MainWindow::setTransitionOnSelectedClip);

    connect(timeline_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        QMenu menu(this);
        const QPoint global = timeline_->viewport()->mapToGlobal(position);
        auto add = [&menu, this](const QString& text, const std::function<void()>& handler) {
            auto* action = menu.addAction(text);
            connect(action, &QAction::triggered, this, [handler] { handler(); });
        };
        add(QStringLiteral("Split at Playhead"), [this] { splitSelectedClip(); });
        add(QStringLiteral("Delete Clip"), [this] { deleteSelectedClip(); });
        add(QStringLiteral("Ripple Delete Clip"), [this] { rippleDeleteSelectedClip(); });
        menu.addSeparator();
        add(QStringLiteral("Trim Start to Playhead"), [this] { trimSelectedClipStart(); });
        add(QStringLiteral("Trim End to Playhead"), [this] { trimSelectedClipEnd(); });
        add(QStringLiteral("Set Speed…"), [this] { setSelectedClipSpeed(); });
        add(QStringLiteral("Add Effect…"), [this] { addEffectToSelectedClip(); });
        add(QStringLiteral("Set Transition…"), [this] { setTransitionOnSelectedClip(); });
        menu.exec(global);
    });
}

void MainWindow::splitSelectedClip() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            auto& clip = track.clips()[ci];
            if (QString::fromStdString(clip.id().toString()) != id) continue;
            const auto clipEnd = clip.start() + clip.duration();
            const double relative = std::max(0.0, static_cast<double>(player_->position()) / 1000.0 - clip.sourceIn().seconds());
            const auto playhead = clip.start() + ccos::core::Time::fromSeconds(relative);
            if (playhead <= clip.start() || playhead >= clipEnd) {
                statusLabel_->setText(QStringLiteral("Playhead is outside the selected clip"));
                return;
            }
            if (commandStack_.execute(std::make_unique<ccos::timeline::SplitClipCommand>(track, ci, playhead))) {
                setDirty(true);
                refreshTimeline();
                statusLabel_->setText(QStringLiteral("Clip split at playhead"));
            }
            return;
        }
    }
}

void MainWindow::deleteSelectedClip() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            if (QString::fromStdString(track.clips()[ci].id().toString()) != id) continue;
            if (commandStack_.execute(std::make_unique<ccos::timeline::DeleteClipCommand>(track, ci))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Clip deleted"));
            }
            return;
        }
    }
}

void MainWindow::rippleDeleteSelectedClip() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            if (QString::fromStdString(track.clips()[ci].id().toString()) != id) continue;
            if (commandStack_.execute(std::make_unique<ccos::timeline::RippleDeleteClipCommand>(track, ci))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Ripple delete applied"));
            }
            return;
        }
    }
}

void MainWindow::trimSelectedClipStart() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            auto& clip = track.clips()[ci];
            if (QString::fromStdString(clip.id().toString()) != id) continue;
            const double relative = std::max(0.0, static_cast<double>(player_->position()) / 1000.0 - clip.sourceIn().seconds());
            const auto newIn = clip.sourceIn() + ccos::core::Time::fromSeconds(relative);
            if (newIn >= clip.sourceOut()) { statusLabel_->setText(QStringLiteral("Invalid trim position")); return; }
            if (commandStack_.execute(std::make_unique<ccos::timeline::TrimClipCommand>(track, ci, newIn, clip.sourceOut()))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Trimmed clip start"));
            }
            return;
        }
    }
}

void MainWindow::trimSelectedClipEnd() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            auto& clip = track.clips()[ci];
            if (QString::fromStdString(clip.id().toString()) != id) continue;
            const double relative = std::max(0.0, static_cast<double>(player_->position()) / 1000.0 - clip.sourceIn().seconds());
            const auto newOut = clip.sourceIn() + ccos::core::Time::fromSeconds(relative);
            if (newOut <= clip.sourceIn() || newOut > clip.sourceOut()) { statusLabel_->setText(QStringLiteral("Invalid trim position")); return; }
            if (commandStack_.execute(std::make_unique<ccos::timeline::TrimClipCommand>(track, ci, clip.sourceIn(), newOut))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Trimmed clip end"));
            }
            return;
        }
    }
}

void MainWindow::nudgeSelectedClipLeft() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) return;
    const QString id = item->toolTip(0);
    const auto delta = ccos::core::Time::fromFrames(1, 30);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            auto& clip = track.clips()[ci];
            if (QString::fromStdString(clip.id().toString()) != id) continue;
            if (clip.start() < delta) { statusLabel_->setText(QStringLiteral("Clip cannot move before timeline start")); return; }
            if (commandStack_.execute(std::make_unique<ccos::timeline::MoveClipCommand>(track, ci, clip.start() - delta))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Clip nudged left"));
            }
            return;
        }
    }
}

void MainWindow::nudgeSelectedClipRight() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) return;
    const QString id = item->toolTip(0);
    const auto delta = ccos::core::Time::fromFrames(1, 30);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            auto& clip = track.clips()[ci];
            if (QString::fromStdString(clip.id().toString()) != id) continue;
            if (commandStack_.execute(std::make_unique<ccos::timeline::MoveClipCommand>(track, ci, clip.start() + delta))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Clip nudged right"));
            }
            return;
        }
    }
}

void MainWindow::setSelectedClipSpeed() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            auto& clip = track.clips()[ci];
            if (QString::fromStdString(clip.id().toString()) != id) continue;
            bool ok = false;
            const double speed = QInputDialog::getDouble(this, QStringLiteral("Clip Speed"), QStringLiteral("Playback speed:"), clip.speed(), 0.25, 4.0, 2, &ok);
            if (!ok) return;
            if (commandStack_.execute(std::make_unique<ccos::timeline::SetClipSpeedCommand>(track, ci, speed))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Speed set to %1x").arg(speed, 0, 'f', 2));
            }
            return;
        }
    }
}

void MainWindow::addEffectToSelectedClip() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    const QStringList ids = ccos::effects::BuiltinEffects::ids();
    QStringList names;
    for (const auto& effectId : ids) names << ccos::effects::BuiltinEffects::get(effectId).name;
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, QStringLiteral("Add Effect"), QStringLiteral("Effect:"), names, 0, false, &ok);
    if (!ok) return;
    const int index = names.indexOf(chosen);
    if (index < 0 || index >= ids.size()) return;
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            if (QString::fromStdString(track.clips()[ci].id().toString()) != id) continue;
            if (commandStack_.execute(std::make_unique<ccos::timeline::AddEffectCommand>(track, ci, ids.at(index)))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Effect added: %1").arg(chosen));
            }
            return;
        }
    }
}

void MainWindow::setTransitionOnSelectedClip() {
    auto* item = timeline_ ? timeline_->currentItem() : nullptr;
    if (!item || !item->parent()) { statusLabel_->setText(QStringLiteral("Select a timeline clip first")); return; }
    const QString id = item->toolTip(0);
    const QStringList transitionIds = {QStringLiteral("cut"), QStringLiteral("fade"), QStringLiteral("dissolve"), QStringLiteral("dip_to_black"), QStringLiteral("wipe"), QStringLiteral("slide"), QStringLiteral("zoom")};
    const QStringList names = {QStringLiteral("Cut"), QStringLiteral("Fade"), QStringLiteral("Dissolve"), QStringLiteral("Dip to Black"), QStringLiteral("Wipe"), QStringLiteral("Slide"), QStringLiteral("Zoom")};
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, QStringLiteral("Transition"), QStringLiteral("Transition:"), names, 0, false, &ok);
    if (!ok) return;
    const int index = names.indexOf(chosen);
    if (index < 0) return;
    qint64 duration = 0;
    if (index != 0) {
        duration = QInputDialog::getInt(this, QStringLiteral("Transition Duration"), QStringLiteral("Duration (ms):"), 500, 50, 10000, 50, &ok);
        if (!ok) return;
    }
    for (auto& track : project_.timeline().tracks()) {
        for (std::size_t ci = 0; ci < track.clips().size(); ++ci) {
            if (QString::fromStdString(track.clips()[ci].id().toString()) != id) continue;
            if (commandStack_.execute(std::make_unique<ccos::timeline::SetTransitionCommand>(track, ci, transitionIds.at(index), duration))) {
                setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Transition set: %1").arg(chosen));
            }
            return;
        }
    }
}

} // namespace ccos::ui

Q_COREAPP_STARTUP_FUNCTION(ccos::ui::initializeAdvancedTimelineUi)
