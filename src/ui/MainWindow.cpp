#include "ui/MainWindow.hpp"
#include "media/MediaImporter.hpp"
#include "project/ProjectSerializer.hpp"
#include "render/FfmpegExporter.hpp"
#include "render/RenderExecutor.hpp"
#include "effects/BuiltinEffects.hpp"
#include "timeline/EditCommands.hpp"
#include <QAction>
#include <QApplication>
#include <QAudioOutput>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>
#include <limits>
#include <memory>

namespace ccos::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("CCOS — Open Source Video Editor"));
    resize(1440, 900);
    project_ = ccos::project::Project(QStringLiteral("Untitled Project"));

    player_ = new QMediaPlayer(this);
    audioOutput_ = new QAudioOutput(this);
    player_->setAudioOutput(audioOutput_);
    audioOutput_->setVolume(1.0);

    renderExecutor_ = new ccos::render::RenderExecutor(this);

    autosaveTimer_ = new QTimer(this);
    autosaveTimer_->setInterval(60000);
    connect(autosaveTimer_, &QTimer::timeout, this, &MainWindow::autosave);
    autosaveTimer_->start();

    buildUi();
    buildMenus();
    refreshMediaBin();
    refreshTimeline();
    setDirty(false);

    connect(renderExecutor_, &ccos::render::RenderExecutor::progress, this, [this](double seconds) {
        statusLabel_->setText(QStringLiteral("Rendering… %1 s processed").arg(seconds, 0, 'f', 1));
    });
    connect(renderExecutor_, &ccos::render::RenderExecutor::finished, this,
            [this](bool success, const QString& error) {
                statusLabel_->setText(success ? QStringLiteral("Render completed") : QStringLiteral("Render failed"));
                if (!success && !error.isEmpty()) QMessageBox::critical(this, QStringLiteral("Render failed"), error);
            });

    const QDir recoveryDirectory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                 QStringLiteral("/Recovery"));
    if (recoveryDirectory.exists()) {
        const QFileInfoList candidates = recoveryDirectory.entryInfoList(
            QStringList() << QStringLiteral("*.ccos"), QDir::Files | QDir::Readable, QDir::Time);
        if (!candidates.isEmpty()) {
            const QString recovery = candidates.first().absoluteFilePath();
            const auto answer = QMessageBox::question(
                this,
                QStringLiteral("Recover Project"),
                QStringLiteral("A recoverable project snapshot was found from %1. Recover it?")
                    .arg(candidates.first().lastModified().toLocalTime().toString(Qt::DefaultLocaleShortDate)));
            if (answer == QMessageBox::Yes) {
                QString error;
                ccos::project::Project recovered;
                if (ccos::project::ProjectSerializer::load(recovered, recovery, &error)) {
                    project_ = std::move(recovered);
                    projectPath_.clear();
                    commandStack_.clear();
                    refreshMediaBin();
                    refreshTimeline();
                    setDirty(true);
                    statusLabel_->setText(QStringLiteral("Recovered autosaved project"));
                } else {
                    statusLabel_->setText(QStringLiteral("Recovery failed: %1").arg(error));
                }
            }
        }
    }
}

QString MainWindow::recoveryPath() const {
    const auto root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString directoryPath = root + QStringLiteral("/Recovery");
    QDir().mkpath(directoryPath);
    const QString projectId = QString::fromStdString(project_.id().toString());
    return QDir(directoryPath).filePath(projectId + QStringLiteral(".ccos"));
}

void MainWindow::setDirty(bool dirty) {
    dirty_ = dirty;
    setWindowTitle(QStringLiteral("CCOS — %1%2")
                       .arg(project_.name(), dirty_ ? QStringLiteral(" *") : QString()));
}

bool MainWindow::confirmDocumentTransition() {
    if (!dirty_) return true;

    const auto answer = QMessageBox::warning(
        this,
        QStringLiteral("Unsaved Changes"),
        QStringLiteral("The project has unsaved changes. Save before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Save) {
        saveProject();
        return !dirty_;
    }

    QFile::remove(recoveryPath());
    setDirty(false);
    return true;
}

void MainWindow::autosave() {
    if (!dirty_) return;
    QString error;
    if (!ccos::project::ProjectSerializer::save(project_, recoveryPath(), &error)) {
        if (statusLabel_) statusLabel_->setText(QStringLiteral("Autosave failed: %1").arg(error));
        return;
    }
    if (statusLabel_) statusLabel_->setText(QStringLiteral("Autosaved"));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!confirmDocumentTransition()) {
        event->ignore();
        return;
    }

    player_->stop();
    if (renderExecutor_->running()) renderExecutor_->cancel();
    autosaveTimer_->stop();
    if (!projectPath_.isEmpty()) QFile::remove(recoveryPath());
    event->accept();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);
    auto* split = new QSplitter(Qt::Horizontal, central);

    auto* left = new QWidget(split);
    auto* leftLayout = new QVBoxLayout(left);
    auto* mediaTitle = new QLabel(QStringLiteral("MEDIA BIN"), left);
    mediaTitle->setStyleSheet(QStringLiteral("font-weight:700; letter-spacing:1px;"));
    leftLayout->addWidget(mediaTitle);
    mediaBin_ = new QListWidget(left);
    mediaBin_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(mediaBin_, &QListWidget::itemSelectionChanged, this, &MainWindow::updateSelection);
    leftLayout->addWidget(mediaBin_, 1);
    auto* addButton = new QPushButton(QStringLiteral("Add to Timeline"), left);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addSelectedToTimeline);
    leftLayout->addWidget(addButton);
    auto* relinkButton = new QPushButton(QStringLiteral("Relink Missing Media"), left);
    connect(relinkButton, &QPushButton::clicked, this, &MainWindow::relinkMissingMedia);
    leftLayout->addWidget(relinkButton);

    auto* center = new QWidget(split);
    auto* centerLayout = new QVBoxLayout(center);
    auto* videoWidget = new QVideoWidget(center);
    videoWidget->setMinimumSize(640, 360);
    videoWidget->setStyleSheet(QStringLiteral("background:#111; border:1px solid #2a2a2a;"));
    player_->setVideoOutput(videoWidget->videoSink());
    previewLabel_ = new QLabel(QStringLiteral("Import a media file to begin"), videoWidget);
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    previewLabel_->setStyleSheet(QStringLiteral("color:#888; font-size:18px; background:transparent;"));
    auto* videoLayout = new QVBoxLayout(videoWidget);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLayout->addWidget(previewLabel_);
    centerLayout->addWidget(videoWidget, 1);

    timelineSlider_ = new QSlider(Qt::Horizontal, center);
    timelineSlider_->setRange(0, 0);
    centerLayout->addWidget(timelineSlider_);
    connect(timelineSlider_, &QSlider::valueChanged, player_, [this](int value) { player_->setPosition(value); });
    connect(player_, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        const qint64 bounded = qBound<qint64>(0, duration, static_cast<qint64>(std::numeric_limits<int>::max()));
        timelineSlider_->setRange(0, static_cast<int>(bounded));
    });
    connect(player_, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!timelineSlider_->isSliderDown()) timelineSlider_->setValue(static_cast<int>(position));
    });

    auto* playback = new QHBoxLayout();
    auto* back = new QPushButton(QStringLiteral("−5s"), center);
    auto* play = new QPushButton(QStringLiteral("▶ / ❚❚"), center);
    auto* forward = new QPushButton(QStringLiteral("+5s"), center);
    playback->addStretch(); playback->addWidget(back); playback->addWidget(play); playback->addWidget(forward); playback->addStretch();
    centerLayout->addLayout(playback);
    connect(play, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(back, &QPushButton::clicked, this, [this] { player_->setPosition(qMax<qint64>(0, player_->position() - 5000)); });
    connect(forward, &QPushButton::clicked, this, [this] { player_->setPosition(qMin(player_->duration(), player_->position() + 5000)); });
    connect(player_, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& message) {
        statusLabel_->setText(QStringLiteral("Playback error: %1").arg(message));
    });

    auto* right = new QWidget(split);
    auto* rightLayout = new QFormLayout(right);
    auto* inspectorTitle = new QLabel(QStringLiteral("INSPECTOR"), right);
    inspectorTitle->setStyleSheet(QStringLiteral("font-weight:700; letter-spacing:1px;"));
    rightLayout->addRow(inspectorTitle);
    rightLayout->addRow(QStringLiteral("Position"), new QLabel(QStringLiteral("0, 0"), right));
    rightLayout->addRow(QStringLiteral("Scale"), new QLabel(QStringLiteral("100%"), right));
    rightLayout->addRow(QStringLiteral("Rotation"), new QLabel(QStringLiteral("0°"), right));
    rightLayout->addRow(QStringLiteral("Opacity"), new QLabel(QStringLiteral("100%"), right));
    rightLayout->addRow(QStringLiteral("Status"), new QLabel(QStringLiteral("Ready"), right));

    split->addWidget(left); split->addWidget(center); split->addWidget(right);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 4); split->setStretchFactor(2, 1);
    root->addWidget(split, 3);
    auto* timelineTitle = new QLabel(QStringLiteral("TIMELINE"), central);
    timelineTitle->setStyleSheet(QStringLiteral("font-weight:700; letter-spacing:1px;"));
    root->addWidget(timelineTitle);
    timeline_ = new QTreeWidget(central);
    timeline_->setHeaderLabels({QStringLiteral("Track / Clip"), QStringLiteral("Start"), QStringLiteral("Duration")});
    timeline_->setAlternatingRowColors(true);
    timeline_->setMinimumHeight(180);
    timeline_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(timeline_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        auto* item = timeline_->itemAt(position);
        if (item == nullptr || item->parent() == nullptr) return;
        timeline_->setCurrentItem(item);

        QMenu menu(this);
        auto add = [this, &menu](const QString& label, void (MainWindow::*slot)()) {
            auto* action = menu.addAction(label);
            connect(action, &QAction::triggered, this, slot);
        };
        add(QStringLiteral("Split Clip…"), &MainWindow::splitSelectedClip);
        add(QStringLiteral("Delete Clip"), &MainWindow::deleteSelectedClip);
        add(QStringLiteral("Ripple Delete"), &MainWindow::rippleDeleteSelectedClip);
        menu.addSeparator();
        add(QStringLiteral("Trim Start…"), &MainWindow::trimSelectedClipStart);
        add(QStringLiteral("Trim End…"), &MainWindow::trimSelectedClipEnd);
        add(QStringLiteral("Nudge Left 0.1s"), &MainWindow::nudgeSelectedClipLeft);
        add(QStringLiteral("Nudge Right 0.1s"), &MainWindow::nudgeSelectedClipRight);
        add(QStringLiteral("Set Speed…"), &MainWindow::setSelectedClipSpeed);
        add(QStringLiteral("Audio Gain / Mute…"), &MainWindow::setSelectedClipAudioMix);
        add(QStringLiteral("Add Effect…"), &MainWindow::addEffectToSelectedClip);
        add(QStringLiteral("Set Transition…"), &MainWindow::setTransitionOnSelectedClip);
        menu.exec(timeline_->viewport()->mapToGlobal(position));
    });
    root->addWidget(timeline_, 1);

    setCentralWidget(central);
    statusLabel_ = new QLabel(QStringLiteral("Ready"), this);
    statusBar()->addPermanentWidget(statusLabel_);

    setStyleSheet(QStringLiteral(
        "QMainWindow { background:#0d0f12; color:#e5e7eb; }"
        "QWidget { color:#e5e7eb; }"
        "QListWidget,QTreeWidget { background:#13161a; border:1px solid #252a31; }"
        "QPushButton { background:#1c2229; border:1px solid #303742; padding:7px 12px; border-radius:5px; }"
        "QPushButton:hover { background:#252c35; }"
        "QMenuBar,QMenu { background:#111418; color:#e5e7eb; }"
        "QMenuBar::item:selected,QMenu::item:selected { background:#242a31; }"
        "QSlider::groove:horizontal { height:4px; background:#2b3138; }"
        "QSlider::handle:horizontal { width:12px; margin:-4px 0; border-radius:6px; background:#e5e7eb; }"));
}

void MainWindow::buildMenus() {
    auto* file = menuBar()->addMenu(QStringLiteral("File"));
    auto* newAction = file->addAction(QStringLiteral("New Project")); newAction->setShortcut(QKeySequence::New); connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    auto* openAction = file->addAction(QStringLiteral("Open Project…")); openAction->setShortcut(QKeySequence::Open); connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    auto* saveAction = file->addAction(QStringLiteral("Save Project")); saveAction->setShortcut(QKeySequence::Save); connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    file->addSeparator();
    auto* importAction = file->addAction(QStringLiteral("Import Media…")); importAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I)); connect(importAction, &QAction::triggered, this, &MainWindow::importMedia);
    auto* relinkAction = file->addAction(QStringLiteral("Relink Missing Media…")); connect(relinkAction, &QAction::triggered, this, &MainWindow::relinkMissingMedia);
    file->addSeparator();
    auto* quitAction = file->addAction(QStringLiteral("Quit")); quitAction->setShortcut(QKeySequence::Quit); connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto* edit = menuBar()->addMenu(QStringLiteral("Edit"));
    auto* undoAction = edit->addAction(QStringLiteral("Undo")); undoAction->setShortcut(QKeySequence::Undo); connect(undoAction, &QAction::triggered, this, &MainWindow::undo);
    auto* redoAction = edit->addAction(QStringLiteral("Redo")); redoAction->setShortcut(QKeySequence::Redo); connect(redoAction, &QAction::triggered, this, &MainWindow::redo);
    menuBar()->addMenu(QStringLiteral("View"));

    auto* exportMenu = menuBar()->addMenu(QStringLiteral("Export"));
    auto* timelineExport = exportMenu->addAction(QStringLiteral("Export Timeline…"));
    connect(timelineExport, &QAction::triggered, this, &MainWindow::exportTimeline);
    auto* selectedExport = exportMenu->addAction(QStringLiteral("Export Selected Media…"));
    connect(selectedExport, &QAction::triggered, this, [this] {
        const int row = mediaBin_->currentRow();
        if (row < 0 || row >= static_cast<int>(project_.assets().size())) { QMessageBox::information(this, QStringLiteral("Export"), QStringLiteral("Select a media asset first.")); return; }
        const QString output = QFileDialog::getSaveFileName(this, QStringLiteral("Export Media"), {}, QStringLiteral("MP4 Video (*.mp4)"));
        if (output.isEmpty()) return;
        QString error; ccos::render::ExportSettings settings;
        if (!ccos::render::FfmpegExporter::exportAsset(project_.assets()[static_cast<std::size_t>(row)], output, settings, QStringLiteral("ffmpeg"), &error)) { QMessageBox::critical(this, QStringLiteral("Export failed"), error); return; }
        statusLabel_->setText(QStringLiteral("Exported: %1").arg(QFileInfo(output).fileName()));
    });
    auto* cancelAction = exportMenu->addAction(QStringLiteral("Cancel Render"));
    cancelAction->setEnabled(false);
    connect(cancelAction, &QAction::triggered, this, &MainWindow::cancelRender);
    connect(timelineExport, &QAction::triggered, this, [cancelAction] { cancelAction->setEnabled(true); });
    connect(renderExecutor_, &ccos::render::RenderExecutor::finished, this, [cancelAction](bool, const QString&) { cancelAction->setEnabled(false); });

    auto* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setMovable(false);
    toolbar->addAction(QStringLiteral("Import"), this, &MainWindow::importMedia);
    toolbar->addAction(QStringLiteral("Relink"), this, &MainWindow::relinkMissingMedia);
    toolbar->addAction(QStringLiteral("Save"), this, &MainWindow::saveProject);
    toolbar->addAction(QStringLiteral("Export"), this, &MainWindow::exportTimeline);
    toolbar->addSeparator();
    toolbar->addAction(QStringLiteral("Undo"), this, &MainWindow::undo);
    toolbar->addAction(QStringLiteral("Redo"), this, &MainWindow::redo);
}

void MainWindow::newProject() {
    if (!confirmDocumentTransition()) return;

    bool ok = false;
    const auto name = QInputDialog::getText(this, QStringLiteral("New Project"), QStringLiteral("Project name:"), QLineEdit::Normal, QStringLiteral("Untitled Project"), &ok);
    if (!ok) return;
    player_->stop();
    commandStack_.clear();
    QFile::remove(recoveryPath());
    project_ = ccos::project::Project(name.trimmed().isEmpty() ? QStringLiteral("Untitled Project") : name.trimmed());
    projectPath_.clear();
    refreshMediaBin();
    refreshTimeline();
    setDirty(false);
    statusLabel_->setText(QStringLiteral("New project created"));
}

QString MainWindow::projectDialogPath(bool save) const {
    if (save) return QFileDialog::getSaveFileName(const_cast<MainWindow*>(this), QStringLiteral("Save Project"), {}, QStringLiteral("CCOS Project (*.ccos);;All Files (*)"));
    return QFileDialog::getOpenFileName(const_cast<MainWindow*>(this), QStringLiteral("Open Project"), {}, QStringLiteral("CCOS Project (*.ccos);;All Files (*)"));
}

void MainWindow::saveProject() {
    if (projectPath_.isEmpty()) projectPath_ = projectDialogPath(true);
    if (projectPath_.isEmpty()) return;
    QString error;
    if (!ccos::project::ProjectSerializer::save(project_, projectPath_, &error)) { QMessageBox::critical(this, QStringLiteral("Save failed"), error); return; }
    QFile::remove(recoveryPath());
    setDirty(false);
    statusLabel_->setText(QStringLiteral("Saved: %1").arg(QFileInfo(projectPath_).fileName()));
}

void MainWindow::openProject() {
    if (!confirmDocumentTransition()) return;

    const auto path = projectDialogPath(false); if (path.isEmpty()) return;
    QString error; ccos::project::Project loaded;
    if (!ccos::project::ProjectSerializer::load(loaded, path, &error)) { QMessageBox::critical(this, QStringLiteral("Open failed"), error); return; }
    QFile::remove(recoveryPath());
    player_->stop();
    commandStack_.clear();
    project_ = std::move(loaded);
    projectPath_ = path;
    refreshMediaBin();
    refreshTimeline();
    setDirty(false);
    statusLabel_->setText(QStringLiteral("Opened: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::importMedia() {
    const auto paths = QFileDialog::getOpenFileNames(this, QStringLiteral("Import Media"), {}, QStringLiteral("Media Files (*.mp4 *.mov *.mkv *.webm *.avi *.wav *.mp3 *.m4a *.png *.jpg *.jpeg);;All Files (*)"));
    if (paths.isEmpty()) return;
    for (auto asset : ccos::media::MediaImporter::importFiles(paths)) project_.addAsset(std::move(asset));
    setDirty(true); refreshMediaBin(); statusLabel_->setText(QStringLiteral("Imported %1 media file(s)").arg(paths.size()));
}

void MainWindow::addSelectedToTimeline() {
    const int row = mediaBin_->currentRow(); if (row < 0 || row >= static_cast<int>(project_.assets().size())) return;
    ccos::timeline::Clip clip(project_.assets()[static_cast<std::size_t>(row)]);
    auto& track = project_.timeline().ensureVideoTrack();
    const auto& clips = track.clips();
    if (!clips.empty()) clip.setStart(clips.back().start() + clips.back().duration());
    if (!commandStack_.execute(std::make_unique<ccos::timeline::AddClipCommand>(track, clip))) return;
    setDirty(true); refreshTimeline(); loadPreviewSource(project_.assets()[static_cast<std::size_t>(row)].path()); statusLabel_->setText(QStringLiteral("Added clip to Video 1"));
}

bool MainWindow::selectedTimelineClip(int* trackIndex, int* clipIndex) const {
    if (!trackIndex || !clipIndex || timeline_ == nullptr) return false;
    const auto* item = timeline_->currentItem();
    if (item == nullptr || item->parent() == nullptr) return false;
    const int track = item->data(0, Qt::UserRole).toInt();
    const int clip = item->data(1, Qt::UserRole).toInt();
    if (track < 0 || clip < 0 ||
        track >= static_cast<int>(project_.timeline().tracks().size()) ||
        clip >= static_cast<int>(project_.timeline().tracks()[static_cast<std::size_t>(track)].clips().size())) {
        return false;
    }
    *trackIndex = track;
    *clipIndex = clip;
    return true;
}

void MainWindow::splitSelectedClip() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
    const double start = clip.start().seconds();
    const double end = (clip.start() + clip.duration()).seconds();
    if (end - start <= 0.002) return;
    bool accepted = false;
    const double defaultPosition = start + (end - start) * 0.5;
    const double position = QInputDialog::getDouble(this, QStringLiteral("Split Clip"),
        QStringLiteral("Timeline position (seconds):"), defaultPosition, start + 0.001, end - 0.001, 3, &accepted);
    if (!accepted) return;
    if (!commandStack_.execute(std::make_unique<ccos::timeline::SplitClipCommand>(
            track, static_cast<std::size_t>(clipIndex), ccos::core::Time::fromSeconds(position)))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Clip split at %1 s").arg(position, 0, 'f', 3));
}

void MainWindow::deleteSelectedClip() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    if (QMessageBox::question(this, QStringLiteral("Delete Clip"),
                              QStringLiteral("Delete the selected clip?")) != QMessageBox::Yes) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    if (!commandStack_.execute(std::make_unique<ccos::timeline::DeleteClipCommand>(
            track, static_cast<std::size_t>(clipIndex)))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Clip deleted"));
}

void MainWindow::rippleDeleteSelectedClip() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    if (!commandStack_.execute(std::make_unique<ccos::timeline::RippleDeleteClipCommand>(
            track, static_cast<std::size_t>(clipIndex)))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Ripple delete applied"));
}

void MainWindow::trimSelectedClipStart() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
    const double sourceIn = clip.sourceIn().seconds();
    const double sourceOut = clip.sourceOut().seconds();
    if (sourceOut - sourceIn <= 0.002) return;
    bool accepted = false;
    const double value = QInputDialog::getDouble(this, QStringLiteral("Trim Start"),
        QStringLiteral("New source-in (seconds):"), sourceIn, sourceIn, sourceOut - 0.001, 3, &accepted);
    if (!accepted) return;
    if (!commandStack_.execute(std::make_unique<ccos::timeline::TrimClipCommand>(
            track, static_cast<std::size_t>(clipIndex), ccos::core::Time::fromSeconds(value), clip.sourceOut()))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Clip start trimmed"));
}

void MainWindow::trimSelectedClipEnd() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
    const double sourceIn = clip.sourceIn().seconds();
    const double sourceOut = clip.sourceOut().seconds();
    if (sourceOut - sourceIn <= 0.002) return;
    bool accepted = false;
    const double value = QInputDialog::getDouble(this, QStringLiteral("Trim End"),
        QStringLiteral("New source-out (seconds):"), sourceOut, sourceIn + 0.001, sourceOut, 3, &accepted);
    if (!accepted) return;
    if (!commandStack_.execute(std::make_unique<ccos::timeline::TrimClipCommand>(
            track, static_cast<std::size_t>(clipIndex), clip.sourceIn(), ccos::core::Time::fromSeconds(value)))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Clip end trimmed"));
}

void MainWindow::nudgeSelectedClipLeft() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
    const auto newStart = ccos::core::Time::fromSeconds(qMax(0.0, clip.start().seconds() - 0.1));
    if (!commandStack_.execute(std::make_unique<ccos::timeline::MoveClipCommand>(
            track, static_cast<std::size_t>(clipIndex), newStart))) return;
    setDirty(true);
    refreshTimeline();
}

void MainWindow::nudgeSelectedClipRight() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
    const auto newStart = clip.start() + ccos::core::Time::fromSeconds(0.1);
    if (!commandStack_.execute(std::make_unique<ccos::timeline::MoveClipCommand>(
            track, static_cast<std::size_t>(clipIndex), newStart))) return;
    setDirty(true);
    refreshTimeline();
}

void MainWindow::setSelectedClipSpeed() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];
    bool accepted = false;
    const double speed = QInputDialog::getDouble(this, QStringLiteral("Clip Speed"),
        QStringLiteral("Playback speed:"), clip.speed(), 0.1, 8.0, 2, &accepted);
    if (!accepted) return;
    if (!commandStack_.execute(std::make_unique<ccos::timeline::SetClipSpeedCommand>(
            track, static_cast<std::size_t>(clipIndex), speed))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Speed set to %1x").arg(speed, 0, 'f', 2));
}

void MainWindow::setSelectedClipAudioMix() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const auto& clip = track.clips()[static_cast<std::size_t>(clipIndex)];

    bool gainAccepted = false;
    const double gain = QInputDialog::getDouble(
        this, QStringLiteral("Audio Gain"),
        QStringLiteral("Linear gain (0 = silent, 1 = unity, 4 = +12 dB):"),
        clip.audioGain(), 0.0, 4.0, 3, &gainAccepted);
    if (!gainAccepted) return;

    bool muteAccepted = false;
    const QString muteChoice = QInputDialog::getItem(
        this, QStringLiteral("Clip Audio"),
        QStringLiteral("State:"), QStringList{QStringLiteral("Unmuted"), QStringLiteral("Muted")},
        clip.audioMuted() ? 1 : 0, false, &muteAccepted);
    if (!muteAccepted) return;

    const bool muted = muteChoice == QStringLiteral("Muted");
    if (!commandStack_.execute(std::make_unique<ccos::timeline::SetClipAudioMixCommand>(
            track, static_cast<std::size_t>(clipIndex), gain, muted))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Audio: %1x%2")
        .arg(gain, 0, 'f', 2)
        .arg(muted ? QStringLiteral(" muted") : QString()));
}

void MainWindow::addEffectToSelectedClip() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const QStringList ids = ccos::effects::BuiltinEffects::ids();
    if (ids.isEmpty()) return;

    bool accepted = false;
    const QString id = QInputDialog::getItem(this, QStringLiteral("Add Effect"),
        QStringLiteral("Effect:"), ids, 0, false, &accepted);
    if (!accepted || id.isEmpty()) return;
    if (!commandStack_.execute(std::make_unique<ccos::timeline::AddEffectCommand>(
            track, static_cast<std::size_t>(clipIndex), id))) {
        QMessageBox::information(this, QStringLiteral("Add Effect"), QStringLiteral("The effect is already present or invalid."));
        return;
    }
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Effect added: %1").arg(id));
}

void MainWindow::setTransitionOnSelectedClip() {
    int trackIndex = -1;
    int clipIndex = -1;
    if (!selectedTimelineClip(&trackIndex, &clipIndex)) return;
    auto& track = project_.timeline().tracks()[static_cast<std::size_t>(trackIndex)];
    const QStringList transitions{
        QStringLiteral("cut"), QStringLiteral("fade"), QStringLiteral("dissolve"),
        QStringLiteral("dip_to_black"), QStringLiteral("wipe"), QStringLiteral("slide"),
        QStringLiteral("zoom")};

    bool accepted = false;
    const QString id = QInputDialog::getItem(this, QStringLiteral("Set Transition"),
        QStringLiteral("Transition:"), transitions, 2, false, &accepted);
    if (!accepted || id.isEmpty()) return;

    bool durationAccepted = false;
    const int duration = QInputDialog::getInt(this, QStringLiteral("Transition Duration"),
        QStringLiteral("Duration (ms):"), 500, 0, 60000, 10, &durationAccepted);
    if (!durationAccepted) return;

    if (!commandStack_.execute(std::make_unique<ccos::timeline::SetTransitionCommand>(
            track, static_cast<std::size_t>(clipIndex), id, duration))) return;
    setDirty(true);
    refreshTimeline();
    statusLabel_->setText(QStringLiteral("Transition set: %1").arg(id));
}

void MainWindow::relinkMissingMedia() {
    const auto missing = project_.missingAssetPaths();
    if (missing.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Relink Missing Media"), QStringLiteral("No missing media was detected."));
        return;
    }

    QString directory = QFileDialog::getExistingDirectory(this, QStringLiteral("Choose Folder with Replacement Media"));
    if (directory.isEmpty()) return;

    int relinked = 0;
    int unresolved = 0;
    for (const auto& oldPath : missing) {
        const QFileInfo oldInfo(oldPath);
        const QString candidate = QDir(directory).filePath(oldInfo.fileName());
        if (!QFileInfo::exists(candidate)) { ++unresolved; continue; }

        for (const auto& asset : project_.assets()) {
            if (asset.path() == oldPath) {
                relinked += project_.relinkAsset(asset.id(), candidate);
                break;
            }
        }
    }

    refreshMediaBin();
    refreshTimeline();
    if (relinked > 0) {
        setDirty(true);
        statusLabel_->setText(QStringLiteral("Relinked %1 media file(s)%2")
                              .arg(relinked)
                              .arg(unresolved > 0 ? QStringLiteral("; %1 unresolved").arg(unresolved) : QString()));
    } else {
        statusLabel_->setText(QStringLiteral("No matching replacement media found"));
        QMessageBox::warning(this, QStringLiteral("Relink Missing Media"),
                             QStringLiteral("No files with matching filenames were found in the selected folder."));
    }
}

void MainWindow::exportTimeline() {
    if (renderExecutor_->running()) { statusLabel_->setText(QStringLiteral("A render is already running")); return; }
    const QString output = QFileDialog::getSaveFileName(this, QStringLiteral("Export Timeline"), {}, QStringLiteral("MP4 Video (*.mp4);;MOV Video (*.mov);;WebM Video (*.webm)"));
    if (output.isEmpty()) return;
    ccos::render::ExportSettings settings;
    settings.container = QFileInfo(output).suffix().toLower();
    if (settings.container.isEmpty()) settings.container = QStringLiteral("mp4");
    if (!renderExecutor_->start(project_, output, settings, QStringLiteral("ffmpeg"))) {
        QMessageBox::critical(this, QStringLiteral("Export"), QStringLiteral("Unable to start the render. Verify FFmpeg is installed and the timeline contains valid media."));
        return;
    }
    statusLabel_->setText(QStringLiteral("Rendering…"));
}

void MainWindow::cancelRender() {
    if (!renderExecutor_->running()) return;
    renderExecutor_->cancel();
    statusLabel_->setText(QStringLiteral("Render cancellation requested"));
}

void MainWindow::undo() { if (commandStack_.undo()) { setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Undo")); } }
void MainWindow::redo() { if (commandStack_.redo()) { setDirty(true); refreshTimeline(); statusLabel_->setText(QStringLiteral("Redo")); } }

void MainWindow::updateSelection() {
    const int row = mediaBin_->currentRow();
    if (row >= 0 && row < static_cast<int>(project_.assets().size())) {
        const auto& asset = project_.assets()[static_cast<std::size_t>(row)];
        previewLabel_->setText(asset.name());
        loadPreviewSource(asset.path());
    }
}

void MainWindow::togglePlayback() {
    if (player_->playbackState() == QMediaPlayer::PlayingState) player_->pause(); else player_->play();
}

void MainWindow::loadPreviewSource(const QString& path) {
    previewLabel_->setVisible(true);
    player_->setSource(QUrl::fromLocalFile(path));
    statusLabel_->setText(QStringLiteral("Preview: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::refreshMediaBin() {
    mediaBin_->clear();
    for (const auto& asset : project_.assets()) {
        auto* item = new QListWidgetItem(asset.name().isEmpty() ? asset.path() : asset.name());
        item->setToolTip(asset.path());
        mediaBin_->addItem(item);
    }
}

void MainWindow::refreshTimeline() {
    timeline_->clear();
    for (std::size_t trackIndex = 0; trackIndex < project_.timeline().tracks().size(); ++trackIndex) {
        const auto& track = project_.timeline().tracks()[trackIndex];
        auto* trackItem = new QTreeWidgetItem(timeline_);
        trackItem->setData(0, Qt::UserRole, static_cast<int>(trackIndex));
        trackItem->setText(0, QStringLiteral("%1  •  %2")
            .arg(track.type() == ccos::timeline::TrackType::Video ? QStringLiteral("V") : QStringLiteral("A"), track.name()));
        for (std::size_t clipIndex = 0; clipIndex < track.clips().size(); ++clipIndex) {
            const auto& clip = track.clips()[clipIndex];
            auto* clipItem = new QTreeWidgetItem(trackItem);
            clipItem->setData(0, Qt::UserRole, static_cast<int>(trackIndex));
            clipItem->setData(1, Qt::UserRole, static_cast<int>(clipIndex));
            clipItem->setText(0, QStringLiteral("Clip %1").arg(QString::fromStdString(clip.id().toString()).left(8)));
            clipItem->setText(1, QString::fromStdString(clip.start().toString()));
            clipItem->setText(2, QString::fromStdString(clip.duration().toString()));
            clipItem->setToolTip(0, QString::fromStdString(clip.id().toString()));
        }
        trackItem->setExpanded(true);
    }
}

} // namespace ccos::ui
