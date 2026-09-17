#include "ui/MainWindow.hpp"

#include "project/ProjectSerializer.hpp"
#include "timeline/EditCommands.hpp"

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
#include <QUuid>

#include <functional>
#include <memory>

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

QString makeWebStoragePath(const QString& fileName, const QString& directory) {
    QDir().mkpath(directory);
    return QDir(directory).filePath(
        QUuid::createUuid().toString(QUuid::WithoutBraces) + QLatin1Char('_') + safeFileName(fileName));
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

    connect(player_, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        const qint64 bounded = qBound<qint64>(0, duration, std::numeric_limits<int>::max());
        timelineSlider_->setRange(0, static_cast<int>(bounded));
        const int row = mediaBin_->currentRow();
        if (row >= 0 && row < static_cast<int>(project_.assets().size())) {
            project_.assets()[static_cast<std::size_t>(row)].metadata().durationMs = duration;
        }
    });
    connect(player_, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!timelineSlider_->isSliderDown()) timelineSlider_->setValue(static_cast<int>(qBound<qint64>(0, position, std::numeric_limits<int>::max())));
    });
    connect(player_, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& message) {
        statusLabel_->setText(QStringLiteral("Playback error: %1").arg(message));
    });

    const QDir recoveryDirectory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/Recovery"));
    if (recoveryDirectory.exists()) {
        const QFileInfoList candidates = recoveryDirectory.entryInfoList(
            QStringList() << QStringLiteral("*.ccos"), QDir::Files | QDir::Readable, QDir::Time);
        if (!candidates.isEmpty()) {
            QString error;
            ccos::project::Project recovered;
            if (ccos::project::ProjectSerializer::load(recovered, candidates.first().absoluteFilePath(), &error)) {
                const auto answer = QMessageBox::question(
                    this, QStringLiteral("Recover Project"),
                    QStringLiteral("A recoverable project snapshot was found. Recover it?"));
                if (answer == QMessageBox::Yes) {
                    project_ = std::move(recovered);
                    projectPath_.clear();
                    commandStack_.clear();
                    refreshMediaBin();
                    refreshTimeline();
                    setDirty(true);
                }
            }
        }
    }
}

QString MainWindow::recoveryPath() const {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString directory = root + QStringLiteral("/Recovery");
    QDir().mkpath(directory);
    return QDir(directory).filePath(QString::fromStdString(project_.id().toString()) + QStringLiteral(".ccos"));
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
    QFile::remove(recoveryPath());
    setDirty(false);
    return true;
}

void MainWindow::autosave() {
    if (!dirty_) return;
    QString error;
    if (!ccos::project::ProjectSerializer::save(project_, recoveryPath(), &error)) {
        statusLabel_->setText(QStringLiteral("Autosave failed: %1").arg(error));
        return;
    }
    statusLabel_->setText(QStringLiteral("Autosaved locally in this browser session"));
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
    leftLayout->addWidget(mediaBin_, 1);
    connect(mediaBin_, &QListWidget::itemSelectionChanged, this, &MainWindow::updateSelection);
    auto* addButton = new QPushButton(QStringLiteral("Add to Timeline"), left);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addSelectedToTimeline);
    leftLayout->addWidget(addButton);

    auto* center = new QWidget(split);
    auto* centerLayout = new QVBoxLayout(center);
    auto* videoWidget = new QVideoWidget(center);
    videoWidget->setMinimumSize(640, 360);
    videoWidget->setStyleSheet(QStringLiteral("background:#111; border:1px solid #2a2a2a;"));
    player_->setVideoOutput(videoWidget);
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
    playback->addStretch(); playback->addWidget(back); playback->addWidget(play); playback->addWidget(forward); playback->addStretch();
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
    rightLayout->addRow(QStringLiteral("Status"), new QLabel(QStringLiteral("Browser / WebAssembly"), right));

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
    root->addWidget(timeline_, 1);

    setCentralWidget(central);
    statusLabel_ = new QLabel(QStringLiteral("Ready — WebAssembly"), this);
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
    auto* quitAction = file->addAction(QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto* edit = menuBar()->addMenu(QStringLiteral("Edit"));
    auto* undoAction = edit->addAction(QStringLiteral("Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &MainWindow::undo);
    auto* redoAction = edit->addAction(QStringLiteral("Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::redo);

    auto* exportMenu = menuBar()->addMenu(QStringLiteral("Export"));
    auto* projectExport = exportMenu->addAction(QStringLiteral("Download Project (.ccos)…"));
    connect(projectExport, &QAction::triggered, this, &MainWindow::saveProject);

    auto* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setMovable(false);
    toolbar->addAction(QStringLiteral("Import"), this, &MainWindow::importMedia);
    toolbar->addAction(QStringLiteral("Save"), this, &MainWindow::saveProject);
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
    commandStack_.clear();
    QFile::remove(recoveryPath());
    project_ = ccos::project::Project(name.trimmed().isEmpty() ? QStringLiteral("Untitled Project") : name.trimmed());
    projectPath_.clear();
    refreshMediaBin();
    refreshTimeline();
    setDirty(false);
}

QString MainWindow::projectDialogPath(bool save) const {
    Q_UNUSED(save);
    return QStringLiteral("/ccos/projects/current.ccos");
}

void MainWindow::saveProject() {
    QString error;
    const QString virtualPath = projectDialogPath(true);
    if (!ccos::project::ProjectSerializer::save(project_, virtualPath, &error)) {
        QMessageBox::critical(this, QStringLiteral("Save failed"), error);
        return;
    }

    QFile file(virtualPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("Save failed"), file.errorString());
        return;
    }
    const QByteArray bytes = file.readAll();
    const QString hint = safeFileName(project_.name()) + QStringLiteral(".ccos");
    QFileDialog::saveFileContent(bytes, hint, this);
    projectPath_ = virtualPath;
    QFile::remove(virtualPath);
    QFile::remove(recoveryPath());
    setDirty(false);
    statusLabel_->setText(QStringLiteral("Project download started"));
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
        QFile::remove(self->recoveryPath());
        self->player_->stop();
        self->commandStack_.clear();
        self->project_ = std::move(loaded);
        self->projectPath_ = path;
        self->refreshMediaBin();
        self->refreshTimeline();
        self->setDirty(false);
        self->statusLabel_->setText(QStringLiteral("Opened: %1").arg(QFileInfo(fileName).fileName()));
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
            self->statusLabel_->setText(QStringLiteral("Imported: %1").arg(QFileInfo(fileName).fileName()));
        }, this);
}

void MainWindow::addSelectedToTimeline() {
    const int row = mediaBin_->currentRow();
    if (row < 0 || row >= static_cast<int>(project_.assets().size())) return;
    ccos::timeline::Clip clip(project_.assets()[static_cast<std::size_t>(row)]);
    auto& track = project_.timeline().ensureVideoTrack();
    const auto& clips = track.clips();
    if (!clips.empty()) clip.setStart(clips.back().start() + clips.back().duration());
    if (!commandStack_.execute(std::make_unique<ccos::timeline::AddClipCommand>(track, clip))) return;
    setDirty(true);
    refreshTimeline();
    loadPreviewSource(project_.assets()[static_cast<std::size_t>(row)].path());
    statusLabel_->setText(QStringLiteral("Added clip to Video 1"));
}

void MainWindow::relinkMissingMedia() {
    const auto missing = project_.missingAssetPaths();
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
                    self->statusLabel_->setText(QStringLiteral("Relinked: %1").arg(QFileInfo(fileName).fileName()));
                }
                break;
            }
        }
    }, this);
}

void MainWindow::exportTimeline() {
    statusLabel_->setText(QStringLiteral("Timeline video export is not available in the browser build yet"));
}

void MainWindow::cancelRender() {
    if (renderExecutor_ && renderExecutor_->running()) renderExecutor_->cancel();
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
    for (const auto& track : project_.timeline().tracks()) {
        auto* trackItem = new QTreeWidgetItem(timeline_);
        trackItem->setText(0, QStringLiteral("%1  •  %2")
            .arg(track.type() == ccos::timeline::TrackType::Video ? QStringLiteral("V") : QStringLiteral("A"), track.name()));
        for (const auto& clip : track.clips()) {
            auto* clipItem = new QTreeWidgetItem(trackItem);
            clipItem->setText(0, QStringLiteral("Clip %1").arg(QString::fromStdString(clip.id().toString()).left(8)));
            clipItem->setText(1, QString::fromStdString(clip.start().toString()));
            clipItem->setText(2, QString::fromStdString(clip.duration().toString()));
            clipItem->setToolTip(0, QString::fromStdString(clip.id().toString()));
        }
        trackItem->setExpanded(true);
    }
}

} // namespace ccos::ui
