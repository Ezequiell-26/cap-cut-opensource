#pragma once
/**
 * CCOS - Timeline UI Widget
 * Widget interactivo de timeline con OpenGL para edición profesional
 * Soporta: drag/drop, trim handles, snapping, herramientas de edición
 * Licencia: MIT
 */

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMouseEvent>
#include <QTimer>
#include <QMenu>
#include <memory>
#include <vector>
#include <optional>

#include "timeline/Timeline.hpp"
#include "timeline/Clip.hpp"
#include "core/Time.hpp"

namespace ccos::ui {

enum class TimelineTool {
    Select,
    Razor,
    Trim,
    Slip,
    Slide,
    Pen,
    Hand,
    Zoom
};

enum class TrimEdge {
    None,
    Left,
    Right
};

enum class SnapTarget {
    None,
    ClipStart,
    ClipEnd,
    Playhead,
    TrackBoundary,
    Marker,
    Keyframe
};

struct SnapPoint {
    ccos::core::Time time;
    SnapTarget type;
    int trackIndex = -1;
    int clipIndex = -1;
    QString label;
};

struct TimelineViewConfig {
    double zoomLevel = 1.0;
    double pixelsPerSecond = 50.0;
    int trackHeight = 80;
    int headerWidth = 120;
    int rulerHeight = 30;
    bool showWaveforms = true;
    bool showThumbnails = true;
    bool showKeyframes = true;
    bool snapEnabled = true;
    double snapThreshold = 10.0; // pixels
    QColor backgroundColor = QColor(30, 30, 30);
    QColor trackBackgroundColor = QColor(40, 40, 40);
    QColor clipColor = QColor(60, 120, 200);
    QColor selectedClipColor = QColor(100, 180, 255);
    QColor playheadColor = QColor(255, 50, 50);
    QColor gridColor = QColor(60, 60, 60);
    QColor textColor = QColor(200, 200, 200);
    QFont trackFont = QFont("Segoe UI", 9);
    QFont timeFont = QFont("Consolas", 8);
};

struct DragState {
    bool isDragging = false;
    bool isTrimming = false;
    bool isSlipping = false;
    bool isSliding = false;
    int draggedTrack = -1;
    int draggedClip = -1;
    TrimEdge trimEdge = TrimEdge::None;
    QPoint startPos;
    ccos::core::Time originalStartTime;
    ccos::core::Time originalDuration;
    ccos::core::Time originalSourceIn;
    std::vector<int> selectedClips;
};

class TimelineWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget() override;

    // Timeline data binding
    void setTimeline(timeline::Timeline* timeline);
    [[nodiscard]] timeline::Timeline* timeline() const { return timeline_; }

    // View configuration
    [[nodiscard]] const TimelineViewConfig& config() const { return config_; }
    void setConfig(const TimelineViewConfig& config);
    void setZoomLevel(double level);
    void setPixelsPerSecond(double pps);
    void fitToContents();
    void scrollToTime(ccos::core::Time time);

    // Playback control
    void setPlayheadTime(ccos::core::Time time);
    [[nodiscard]] ccos::core::Time playheadTime() const { return playheadTime_; }
    void setPlaying(bool playing);
    [[nodiscard]] bool isPlaying() const { return isPlaying_; }

    // Tools
    void setCurrentTool(TimelineTool tool);
    [[nodiscard]] TimelineTool currentTool() const { return currentTool_; }

    // Selection
    void clearSelection();
    void selectClip(int trackIndex, int clipIndex);
    void deselectClip(int trackIndex, int clipIndex);
    [[nodiscard]] const std::vector<std::pair<int, int>>& selectedClips() const { return selectedClips_; }

    // Snapping
    void setSnapEnabled(bool enabled) { config_.snapEnabled = enabled; }
    [[nodiscard]] bool isSnapEnabled() const { return config_.snapEnabled; }
    void setSnapThreshold(double pixels) { config_.snapThreshold = pixels; }

    // Time conversion utilities
    [[nodiscard]] double timeToX(ccos::core::Time time) const;
    [[nodiscard]] ccos::core::Time xToTime(double x) const;
    [[nodiscard]] int trackToY(int trackIndex) const;
    [[nodiscard]] int yToTrack(int y) const;

    // Clipboard operations
    void copySelected();
    void cutSelected();
    void paste();
    void deleteSelected();
    void rippleDeleteSelected();

signals:
    void playheadTimeChanged(ccos::core::Time time);
    void selectionChanged();
    void clipMoved(int trackIndex, int clipIndex, ccos::core::Time newStart);
    void clipTrimmed(int trackIndex, int clipIndex, ccos::core::Time newStart, ccos::core::Time newDuration);
    void clipSplit(int trackIndex, ccos::core::Time time);
    void playStateChanged(bool playing);
    void zoomChanged(double level);
    void contextMenuRequested(QPoint pos, int trackIndex, int clipIndex);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // Rendering
    void renderBackground();
    void renderGrid();
    void renderRuler();
    void renderTrackHeaders();
    void renderTracks();
    void renderClip(const timeline::Clip& clip, int trackIndex, int clipIndex, bool isSelected);
    void renderWaveform(const timeline::Clip& clip, QRectF rect);
    void renderThumbnail(const timeline::Clip& clip, QRectF rect);
    void renderPlayhead();
    void renderWorkArea();
    void renderMarkers();
    void renderKeyframes();
    void renderToolOverlay();
    void renderDragGhost();

    // Interaction
    SnapPoint findSnapPoint(ccos::core::Time time, int trackIndex, int clipIndex);
    ccos::core::Time snapToGrid(ccos::core::Time time);
    void startDrag(QPoint pos, int trackIndex, int clipIndex);
    void updateDrag(QPoint pos);
    void endDrag();
    void splitClipAtPlayhead();
    void razorClipAtPosition(QPoint pos);

    // Helpers
    void updateGeometry();
    void createShaders();
    void createBuffers();
    void loadTextures();
    QRectF clipRect(const timeline::Clip& clip, int trackIndex) const;
    QRectF waveformRect(const timeline::Clip& clip, int trackIndex) const;
    QRectF thumbnailRect(const timeline::Clip& clip, int trackIndex) const;

    // State
    timeline::Timeline* timeline_ = nullptr;
    TimelineViewConfig config_;
    TimelineTool currentTool_ = TimelineTool::Select;
    DragState dragState_;
    ccos::core::Time playheadTime_;
    ccos::core::Time workAreaStart_;
    ccos::core::Time workAreaEnd_;
    std::vector<SnapPoint> snapPoints_;
    std::vector<std::pair<int, int>> selectedClips_;
    std::vector<ccos::core::Time> markers_;

    // OpenGL resources
    QOpenGLShaderProgram* shaderProgram_ = nullptr;
    QOpenGLBuffer vertexBuffer_;
    QOpenGLBuffer indexBuffer_;
    QOpenGLVertexArrayObject vao_;

    // Scroll state
    int scrollOffset_ = 0;
    int horizontalScrollMax_ = 0;

    // Timer for playback
    QTimer* playbackTimer_ = nullptr;
    qint64 lastFrameTime_ = 0;

    // Context menu
    QMenu* contextMenu_ = nullptr;

    // Drag ghost rendering
    bool renderDragGhost_ = false;
    QRectF dragGhostRect_;
    QString dragGhostLabel_;
};

} // namespace ccos::ui
