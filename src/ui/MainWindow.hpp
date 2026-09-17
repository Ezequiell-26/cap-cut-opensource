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

namespace ccos::render { class RenderExecutor; }

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
    void exportTimeline();
    void cancelRender();
    void relinkMissingMedia();
    void splitSelectedClip();
    void deleteSelectedClip();
    void rippleDeleteSelectedClip();
    void trimSelectedClipStart();
    void trimSelectedClipEnd();
    void nudgeSelectedClipLeft();
    void nudgeSelectedClipRight();
    void setSelectedClipSpeed();
    void addEffectToSelectedClip();
    void setTransitionOnSelectedClip();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    bool selectedTimelineClip(int* trackIndex, int* clipIndex) const;
    void buildUi();
    void buildMenus();
    void refreshMediaBin();
    void refreshTimeline();
    QString projectDialogPath(bool save) const;
    void loadPreviewSource(const QString& path);
    QString recoveryPath() const;
    void setDirty(bool dirty);
    bool confirmDocumentTransition();

    ccos::project::Project project_;
    ccos::core::CommandStack commandStack_;
    QString projectPath_;
    bool dirty_ = false;
    QListWidget* mediaBin_ = nullptr;
    QTreeWidget* timeline_ = nullptr;
    QLabel* previewLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QSlider* timelineSlider_ = nullptr;
    QMediaPlayer* player_ = nullptr;
    QAudioOutput* audioOutput_ = nullptr;
    QTimer* autosaveTimer_ = nullptr;
    ccos::render::RenderExecutor* renderExecutor_ = nullptr;
};
}
