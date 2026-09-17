#include "ui/MainWindow.hpp"

#include "project/ProjectSerializer.hpp"
#include "render/ExportPresets.hpp"
#include "timeline/EditCommands.hpp"
#include "web/BrowserStorage.hpp"
#include "web/WebFfmpegRenderer.hpp"

#include <QAbstractItemView>
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
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSaveFile>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

namespace ccos::ui {
namespace {

constexpr qsizetype kMaxWebMediaBytes = 512LL * 1024 * 1024;
constexpr qsizetype kMaxWebProjectBytes = 32LL * 1024 * 1024;

QString safeFileName(QString name) {
    name = QFileInfo(name).fileName().trimmed();
    if (name.isEmpty()) name = QStringLiteral("media.bin");
    for (QChar& c : name) {
        const bool allowed = c.isLetterOrNumber() || c == QLatin1Char('.') || c == QLatin1Char('-') || c == QLatin1Char('_');
        if (!allowed) c = QLatin1Char('_');
    }
    return name.left(128);
}

QString idString(const ccos::core::Uuid& id) {
    return QString::fromStdString(id.toString());
}

QString stableProjectPath(const ccos::project::Project& project) {
    return QStringLiteral("/ccos/projects/%1.ccos").arg(idString(project.id()));
}

QString recoveryProjectPath(const ccos::project::Project& project) {
    return QStringLiteral("/ccos/recovery/%1.ccos").arg(idString(project.id()));
}

QString makeWebStoragePath(const QString& fileName, const QString& directory) {
    QDir().mkpath(directory);
    return QDir(directory).filePath(
        idString(ccos::core::Uuid{}) + QLatin1Char('_') + safeFileName(fileName));
}

bool writeWebBytes(const QString& path, const QByteArray& bytes, qsizetype maximum, QString* error) {
    if (bytes.isEmpty()) {
        if (error) *error = QStringLiteral("Selected file is empty");
        return false;
    }
    if (bytes.size() > maximum) {
        if (error) *error = QStringLiteral("Selected file exceeds the browser import limit (%1 MiB)")
            .arg(maximum / (1024 * 1024));
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("CCOS — Open Source Video Editor"));
    resize(1440, 900);
    project_ = ccos::project::Project(QStringLiteral("Untitled Project"));

    player_ = new QMediaPlayer(this);
    audioOutput_ = new QAudioOutput(this);
    player_->setAudioOutput(audioOutput_);
    audioOutput_->setVolume(1.0);

    autosaveTimer_ = new QTimer(this);
    autosaveTimer_->setInterval(10000);
    connect(autosaveTimer_, &QTimer::timeout, this, &MainWindow::autosave);
    autosaveTimer_->start();

    buildUi();
    buildMenus();
    refreshMediaBin();
    refreshTimeline();
    setDirty(false);
    statusLabel_->setText(QStringLiteral("Initializing browser storage…"));

    connect(player_, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        const qint64 bounded = qBound<qint64>(0, duration, std::numeric_limits<int>::max());
        timelineSlider_->setRange(0, static_cast<int>(bounded));
        const int row = mediaBin_->currentRow();
        if (row >= 0 && row < static_cast<int>(project_.assets().size())) {
            project_.assets()[static_cast<std::size_t>(row)].metadata().durationMs = duration;
            setDirty(true);
        }
    });
    connect(player_, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!timelineSlider_->isSliderDown()) {
            timelineSlider_->setValue(static_cast<int>(qBound<qint64>(0, position, std::numeric_limits<int>::max())));
        }
    });
    connect(player_, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& message) {
        previewLabel_->setText(QStringLiteral("Playback error: %1").arg(message));
        previewLabel_->setVisible(true);
        statusLabel_->setText(message);
    });

    ccos::web::BrowserStorage::initialize([this](bool ok, const QString& error) {
        if (!ok) {
            statusLabel_->setText(QStringLiteral("Persistent storage unavailable: %1").arg(error));
            return;
        }
        QDir().mkpath(QStringLiteral("/ccos/media"));
        QDir().mkpath(QStringLiteral("/ccos/projects"));
        QDir().mkpath(QStringLiteral("/ccos/recovery"));
        statusLabel_->setText(QStringLiteral("Browser storage ready"));

        const QDir recovery(QStringLiteral("/ccos/recovery"));
        const QFileInfoList candidates = recovery.entryInfoList(QStringList() << QStringLiteral("*.ccos"),
                                                                 QDir::Files | QDir::Readable, QDir::Time);
        if (candidates.isEmpty()) return;

        ccos::project::Project recovered;
        QString loadError;
        if (!ccos::project::ProjectSerializer::load(recovered, candidates.first().absoluteFilePath(), &loadError)) {
            statusLabel_->setText(QStringLiteral("Recovery snapshot ignored: %1").arg(loadError));
            return;
        }
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Recover Project"),
            QStringLiteral("A recoverable project snapshot was found. Recover it?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::Yes) return;

        player_->stop();
        commandStack_.clear();
        project_ = std::move(recovered);
        projectPath_ = stableProjectPath(project_);
        refreshMediaBin();
        refreshTimeline();
        updateSelection();
        setDirty(true);
        statusLabel_->setText(QStringLiteral("Recovered project"));
    });
}

void MainWindow::setDirty(bool dirty) {
    dirty_ = dirty;
    setWindowTitle(QStringLiteral("CCOS — %1%2").arg(project_.name(), dirty_ ? QStringLiteral(" *") : QString()));
}

bool MainWindow::confirmDocumentTransition() {
    if (!dirty_) return true;
    const auto answer = QMessageBox::warning(
        this, QStringLiteral("Unsaved Changes"),
        QStringLiteral("The project has unsaved changes. Save before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Save) {
        saveProject();
        return !dirty_;
    }
    QFile::remove(recoveryProjectPath(project_));
    setDirty(false);
    return true;
}

void MainWindow::autosave() {
    if (!dirty_) return;
    QString error;
    const QString path = recoveryProjectPath(project_);
    if (!ccos::project::ProjectSerializer::save(project_, path, &error)) {
        statusLabel_->setText(QStringLiteral("Autosave failed: %1").arg(error));
        return;
    }
    ccos::web::BrowserStorage::sync();
    statusLabel_->setText(QStringLiteral("Autosaved locally"));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!confirmDocumentTransition()) {
        event->ignore();
        return;
    }
    player_->stop();
    autosaveTimer_->stop();
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
    leftLayout->addWidget(mediaBin_, 1);
    connect(mediaBin_, &QListWidget::itemSelectionChanged, this, &MainWindow::updateSelection);
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
    connect(timelineSlider_, &QSlider::valueChanged, player_, &QMediaPlayer::setPosition);

    auto* playback = new QHBoxLayout();
    auto* back = new QPushButton(QStringLiteral("−5s"), center);
    auto* play = new QPushButton(QStringLiteral("▶ / ❚❚"), center);
    auto* forward = new QPushButton(QStringLiteral("+5s"), center);
    playback->addStretch();
    playback->addWidget(back);
    playback->addWidget(play);
    playback->addWidget(forward);
    playback->addStretch();
    centerLayout->addLayout(playback);
    connect(play, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(back, &QPushButton::clicked, this, [this] { player_->setPosition(qMax<qint64>(0, player_->position() - 5000)); });
    connect(forward, &QPushButton::clicked, this, [this] { player_->setPosition(qMin(player_->duration(), player_->position() + 5000)); });

    auto* right = new QWidget(split);
    auto* rightLayout = new QFormLayout(right);
    auto* inspectorTitle = new QLabel(QStringLiteral("INSPECTOR"), right);
    inspectorTitle->setStyleSheet(QStringLiteral("font-weight:700; letter-spacing:1px;"));
    rightLayout->addRow(inspectorTitle);
    rightLayout->addRow(QStringLiteral("Position"), new QLabel(QStringLiteral("0, 0"), right));
    rightLayout->addRow(QStringLiteral("Scale"), new QLabel(QStringLiteral("100%"), right));
    rightLayout->addRow(QStringLiteral("Rotation"), new QLabel(QStringLiteral("0°"), right));
    rightLayout->addRow(QStringLiteral("Opacity"), new QLabel(QStringLiteral("100%"), right));
    rightLayout->addRow(QStringLiteral("Engine"), new QLabel(QStringLiteral("C++ / Qt / WebAssembly"), right));
    rightLayout->addRow(QStringLiteral("Storage"), new QLabel(QStringLiteral("Persistent browser storage"), right));

    split->addWidget(left);
    split->addWidget(center);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 4);
    split->setStretchFactor(2, 1);
    root->addWidget(split, 3);

    auto* timelineTitle = new QLabel(QStringLiteral("TIMELINE"), central);
    timelineTitle->setStyleSheet(QStringLiteral("font-weight:700; letter-spacing:1px;"));
    root->addWidget(timelineTitle);
    timeline_ = new QTreeWidget(central);
    timeline_->setHeaderLabels({QStringLiteral("Track / Clip"), QStringLiteral("Start"), QStringLiteral("Duration")});
    timeline_->setAlternatingRowColors(true);
    timeline_->setMinimumHeight(180);
    root->addWidget(timeline_, 1);

    setCentralWidget(central);
    statusLabel_ = new QLabel(QStringLiteral("Starting…"), this);
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
    auto* newAction = file->addAction(QStringLiteral("New Project"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    auto* openAction = file->addAction(QStringLiteral("Open Project…"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    auto* saveAction = file->addAction(QStringLiteral("Save Project"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    file->addSeparator();
    auto* importAction = file->addAction(QStringLiteral("Import Media…"));
    connect(importAction, &QAction::triggered, this, &MainWindow::importMedia);
    file->addSeparator();
    auto* quitAction = file->addAction(QStringLiteral("Close Editor"));
    connect(quitAction, &QAction::triggered, this, [this] { close(); });

    auto* edit = menuBar()->addMenu(QStringLiteral("Edit"));
    auto* undoAction = edit->addAction(QStringLiteral("Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &MainWindow::undo);
    auto* redoAction = edit->addAction(QStringLiteral("Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::redo);

    auto* exportMenu = menuBar()->addMenu(QStringLiteral("Export"));
    auto* exportVideo = exportMenu->addAction(QStringLiteral("Export Video…"));
    connect(exportVideo, &QAction::triggered, this, &MainWindow::exportTimeline);
    auto* projectExport = exportMenu->addAction(QStringLiteral("Download Project (.ccos)…"));
    connect(projectExport, &QAction::triggered, this, &MainWindow::saveProject);
    auto* cancelExport = exportMenu->addAction(QStringLiteral("Cancel Export"));
    connect(cancelExport, &QAction::triggered, this, &MainWindow::cancelRender);

    auto* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setMovable(false);
    toolbar->addAction(QStringLiteral("Import"), this, &MainWindow::importMedia);
    toolbar->addAction(QStringLiteral("Save"), this, &MainWindow::saveProject);
    toolbar->addAction(QStringLiteral("Export"), this, &MainWindow::exportTimeline);
    toolbar->addSeparator();
    toolbar->addAction(QStringLiteral("Undo"), this, &MainWindow::undo);
    toolbar->addAction(QStringLiteral("Redo"), this, &MainWindow::redo);
}

void MainWindow::newProject() {
    if (!confirmDocumentTransition()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("New Project"), QStringLiteral("Project name:"), QLineEdit::Normal, QStringLiteral("Untitled Project"), &ok);
    if (!ok) return;
    player_->stop();
    QFile::remove(recoveryProjectPath(project_));
    commandStack_.clear();
    project_ = ccos::project::Project(name.trimmed().isEmpty() ? QStringLiteral("Untitled Project") : name.trimmed());
    projectPath_.clear();
    refreshMediaBin();
    refreshTimeline();
    setDirty(false);
    statusLabel_->setText(QStringLiteral("New project created"));
}

QString MainWindow::projectDialogPath(bool save) const {
    Q_UNUSED(save);
    return projectPath_.isEmpty() ? stableProjectPath(project_) : projectPath_;
}

void MainWindow::saveProject() {
    const QString path = projectDialogPath(true);
    QString error;
    if (!ccos::project::ProjectSerializer::save(project_, path, &error)) {
        QMessageBox::critical(this, QStringLiteral("Save failed"), error);
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("Save failed"), file.errorString());
        return;
    }
    const QByteArray bytes = file.readAll();
    QFileDialog::saveFileContent(bytes, safeFileName(project_.name()) + QStringLiteral(".ccos"), this);
    projectPath_ = path;
    QFile::remove(recoveryProjectPath(project_));
    setDirty(false);
    ccos::web::BrowserStorage::sync([this](bool ok, const QString& message) {
        if (!ok) statusLabel_->setText(QStringLiteral("Saved in memory, browser persistence failed: %1").arg(message));
    });
    statusLabel_->setText(QStringLiteral("Project saved and download started"));
}

void MainWindow::openProject() {
    if (!confirmDocumentTransition()) return;
    QPointer<MainWindow> self(this);
    QFileDialog::getOpenFileContent(QStringLiteral("CCOS Project (*.ccos)"), [self](const QString& fileName, const QByteArray& bytes) {
        if (!self || fileName.isEmpty()) return;
        if (bytes.isEmpty() || bytes.size() > kMaxWebProjectBytes) {
            self->statusLabel_->setText(QStringLiteral("Project file is empty or exceeds the 32 MiB browser limit"));
            return;
        }
        const QString path = makeWebStoragePath(fileName, QStringLiteral("/ccos/projects"));
        QString error;
        if (!writeWebBytes(path, bytes, kMaxWebProjectBytes, &error)) {
            self->statusLabel_->setText(QStringLiteral("Open failed: %1").arg(error));
            return;
        }
        ccos::project::Project loaded;
        if (!ccos::project::ProjectSerializer::load(loaded, path, &error)) {
            QFile::remove(path);
            self->statusLabel_->setText(QStringLiteral("Open failed: %1").arg(error));
            return;
        }
        QFile::remove(recoveryProjectPath(self->project_));
        self->player_->stop();
        self->commandStack_.clear();
        self->project_ = std::move(loaded);
        self->projectPath_ = path;
        self->refreshMediaBin();
        self->refreshTimeline();
        self->setDirty(false);
        self->statusLabel_->setText(QStringLiteral("Opened: %1").arg(QFileInfo(fileName).fileName()));
        ccos::web::BrowserStorage::sync();
    });
}

void MainWindow::importMedia() {
    QPointer<MainWindow> self(this);
    QFileDialog::getOpenFileContent(
        QStringLiteral("Media Files (*.mp4 *.mov *.mkv *.webm *.avi *.wav *.mp3 *.m4a *.png *.jpg *.jpeg)"),
        [self](const QString& fileName, const QByteArray& bytes) {
            if (!self || fileName.isEmpty()) return;
            if (bytes.size() > kMaxWebMediaBytes) {
                self->statusLabel_->setText(QStringLiteral("Media file exceeds the 512 MiB browser limit"));
                return;
            }
            const QString path = makeWebStoragePath(fileName, QStringLiteral("/ccos/media"));
            QString error;
            if (!writeWebBytes(path, bytes, kMaxWebMediaBytes, &error)) {
                self->statusLabel_->setText(QStringLiteral("Import failed: %1").arg(error));
                return;
            }
            ccos::media::MediaAsset asset(path);
            asset.setName(QFileInfo(fileName).fileName());
            self->project_.addAsset(std::move(asset));
            self->setDirty(true);
            self->refreshMediaBin();
            self->mediaBin_->setCurrentRow(self->mediaBin_->count() - 1);
            ccos::web::BrowserStorage::sync();
            self->statusLabel_->setText(QStringLiteral("Imported: %1").arg(QFileInfo(fileName).fileName()));
        }, this);
}

void MainWindow::addSelectedToTimeline() {
    const int row = mediaBin_->currentRow();
    if (row < 0 || row >= static_cast<int>(project_.assets().size())) return;
    ccos::timeline::Clip clip(project_.assets()[static_cast<std::size_t>(row)]);
    auto& track = project_.timeline().ensureVideoTrack();
    if (!track.clips().empty()) {
        const auto& previous = track.clips().back();
        clip.setStart(previous.start() + previous.duration());
    }
    if (!commandStack_.execute(std::make_unique<ccos::timeline::AddClipCommand>(track, clip))) return;
    setDirty(true);
    refreshTimeline();
    loadPreviewSource(project_.assets()[static_cast<std::size_t>(row)].path());
    statusLabel_->setText(QStringLiteral("Added clip to Video 1"));
}

void MainWindow::relinkMissingMedia() {
    const QStringList missing = project_.missingAssetPaths();
    if (missing.isEmpty()) {
        statusLabel_->setText(QStringLiteral("No missing media detected"));
        return;
    }
    const QString oldPath = missing.first();
    QPointer<MainWindow> self(this);
    QFileDialog::getOpenFileContent(QStringLiteral("Replacement Media (*)"), [self, oldPath](const QString& fileName, const QByteArray& bytes) {
        if (!self || fileName.isEmpty()) return;
        const QString path = makeWebStoragePath(fileName, QStringLiteral("/ccos/media"));
        QString error;
        if (!writeWebBytes(path, bytes, kMaxWebMediaBytes, &error)) {
            self->statusLabel_->setText(QStringLiteral("Relink failed: %1").arg(error));
            return;
        }
        for (const auto& asset : self->project_.assets()) {
            if (asset.path() == oldPath) {
                if (self->project_.relinkAsset(asset.id(), path) > 0) {
                    self->setDirty(true);
                    self->refreshMediaBin();
                    self->refreshTimeline();
                    self->loadPreviewSource(path);
                    ccos::web::BrowserStorage::sync();
                    self->statusLabel_->setText(QStringLiteral("Relinked: %1").arg(QFileInfo(fileName).fileName()));
                }
                break;
            }
        }
    }, this);
}

void MainWindow::exportTimeline() {
    const QList<ccos::render::ExportPreset> presets = ccos::render::ExportPresetCatalog::all();
    QStringList names;
    for (const auto& preset : presets) names << QStringLiteral("%1 — %2").arg(preset.name, preset.description);
    bool ok = false;
    const QString selected = QInputDialog::getItem(this, QStringLiteral("Export Video"), QStringLiteral("Preset:"), names, 0, false, &ok);
    if (!ok || selected.isEmpty()) return;

    const int index = names.indexOf(selected);
    if (index < 0 || index >= presets.size()) return;
    const auto settings = presets.at(index).settings;

    statusLabel_->setText(QStringLiteral("Preparing WebAssembly render…"));
    if (!ccos::web::WebFfmpegRenderer::exportTimeline(project_, settings, [this](bool success, const QString& message) {
            statusLabel_->setText(message);
            if (!success) QMessageBox::warning(this, QStringLiteral("Export failed"), message);
        })) {
        statusLabel_->setText(QStringLiteral("Unable to start browser export"));
    }
}

void MainWindow::cancelRender() {
    ccos::web::WebFfmpegRenderer::cancel();
    statusLabel_->setText(QStringLiteral("Export cancellation requested"));
}

void MainWindow::undo() {
    if (commandStack_.undo()) {
        setDirty(true);
        refreshTimeline();
    }
}

void MainWindow::redo() {
    if (commandStack_.redo()) {
        setDirty(true);
        refreshTimeline();
    }
}

void MainWindow::updateSelection() {
    const int row = mediaBin_->currentRow();
    if (row >= 0 && row < static_cast<int>(project_.assets().size())) {
        const auto& asset = project_.assets()[static_cast<std::size_t>(row)];
        previewLabel_->setText(asset.name());
        loadPreviewSource(asset.path());
    }
}

void MainWindow::togglePlayback() {
    if (player_->playbackState() == QMediaPlayer::PlayingState) player_->pause();
    else player_->play();
}

void MainWindow::loadPreviewSource(const QString& path) {
    previewLabel_->setVisible(false);
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
    for (const auto& track : project_.timeline().tracks()) {
        auto* trackItem = new QTreeWidgetItem(timeline_);
        trackItem->setText(0, QStringLiteral("%1  •  %2")
            .arg(track.type() == ccos::timeline::TrackType::Video ? QStringLiteral("V") : QStringLiteral("A"), track.name()));
        for (const auto& clip : track.clips()) {
            auto* clipItem = new QTreeWidgetItem(trackItem);
            clipItem->setText(0, QStringLiteral("Clip %1").arg(idString(clip.id()).left(8)));
            clipItem->setText(1, QString::fromStdString(clip.start().toString()));
            clipItem->setText(2, QString::fromStdString(clip.duration().toString()));
            clipItem->setToolTip(0, idString(clip.id()));
        }
        trackItem->setExpanded(true);
    }
}

} // namespace ccos::ui
