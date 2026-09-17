#pragma once
#include "core/CommandStack.hpp"
#include "project/Project.hpp"
#include <QMainWindow>

class QListWidget;
class QTreeWidget;
class QLabel;
class QSlider;
class QMediaPlayer;
class QAudioOutput;
class QTimer;
class QCloseEvent;

namespace ccos::ui {
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private Q_SLOTS:
    void newProject();
    void openProject();
    void saveProject();
    void importMedia();
    void addSelectedToTimeline();
    void updateSelection();
    void togglePlayback();
    void undo();
    void redo();
    void autosave();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void buildMenus();
    void refreshMediaBin();
    void refreshTimeline();
    QString projectDialogPath(bool save) const;
    void loadPreviewSource(const QString& path);
    QString recoveryPath() const;

    ccos::project::Project project_;
    ccos::core::CommandStack commandStack_;
    QString projectPath_;
    QListWidget* mediaBin_ = nullptr;
    QTreeWidget* timeline_ = nullptr;
    QLabel* previewLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QSlider* timelineSlider_ = nullptr;
    QMediaPlayer* player_ = nullptr;
    QAudioOutput* audioOutput_ = nullptr;
    QTimer* autosaveTimer_ = nullptr;
};
}
