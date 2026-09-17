#pragma once
#include "project/Project.hpp"
#include <QMainWindow>

class QListWidget;
class QTreeWidget;
class QTableWidget;
class QLabel;
class QSlider;

namespace ccos::ui {
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void importMedia();
    void addSelectedToTimeline();
    void updateSelection();

private:
    void buildUi();
    void buildMenus();
    void refreshMediaBin();
    void refreshTimeline();
    QString projectDialogPath(bool save) const;

    ccos::project::Project project_;
    QString projectPath_;
    QListWidget* mediaBin_ = nullptr;
    QTreeWidget* timeline_ = nullptr;
    QLabel* previewLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QSlider* timelineSlider_ = nullptr;
};
}
