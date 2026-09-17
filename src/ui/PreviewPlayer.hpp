#pragma once
/**
 * CCOS - Preview Player Widget
 * Reproductor de preview frame-accurate con sincronización audio/video
 * Licencia: MIT
 */

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QAudioOutput>
#include <QIODevice>
#include <QTimer>
#include <QMutex>
#include <memory>
#include <vector>

#include "core/Time.hpp"
#include "timeline/Timeline.hpp"
#include "preview/pipeline/RenderPipeline.hpp"

namespace ccos::ui {

struct PreviewConfig {
    int targetWidth = 1920;
    int targetHeight = 1080;
    double targetFPS = 30.0;
    bool enableAudio = true;
    bool loopEnabled = false;
    bool showOverlays = true;
    bool showSafeZones = false;
    bool enableHDR = false;
    float audioVolume = 1.0f;
    QColor backgroundColor = QColor(0, 0, 0);
};

enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
    Seeking,
    Buffering
};

class PreviewPlayer : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit PreviewPlayer(QWidget* parent = nullptr);
    ~PreviewPlayer() override;

    // Timeline binding
    void setTimeline(timeline::Timeline* timeline);
    [[nodiscard]] timeline::Timeline* timeline() const { return timeline_; }

    // Playback control
    void play();
    void pause();
    void stop();
    void seek(ccos::core::Time time);
    void stepForward();
    void stepBack();
    
    [[nodiscard]] PlaybackState playbackState() const { return state_; }
    [[nodiscard]] ccos::core::Time currentTime() const { return currentTime_; }
    [[nodiscard]] ccos::core::Time duration() const { return duration_; }
    
    void setLoopEnabled(bool enabled) { config_.loopEnabled = enabled; }
    [[nodiscard]] bool isLoopEnabled() const { return config_.loopEnabled; }

    // Configuration
    void setConfig(const PreviewConfig& config);
    [[nodiscard]] const PreviewConfig& config() const { return config_; }
    void setVolume(float volume);
    [[nodiscard]] float volume() const { return config_.audioVolume; }
    void setMuted(bool muted);
    [[nodiscard]] bool isMuted() const { return muted_; }

    // Frame stepping
    void stepFrame(int frames);
    
    // Markers
    void addMarker(ccos::core::Time time, const QString& label = {});
    void removeMarker(ccos::core::Time time);
    void clearMarkers();
    [[nodiscard]] const std::vector<std::pair<ccos::core::Time, QString>>& markers() const { return markers_; }

    // Quality settings
    enum class Quality {
        Low,      // Proxy resolution
        Medium,   // Half resolution
        High,     // Full resolution
        Maximum   // Native resolution
    };
    void setQuality(Quality quality);
    [[nodiscard]] Quality quality() const { return currentQuality_; }

    // Overlay options
    void setShowSafeZones(bool show);
    void setShowOverlays(bool show);
    void setShowFocusPeaking(bool show);
    void setShowHistogram(bool show);

signals:
    void frameRendered(ccos::core::Time time);
    void playbackStateChanged(PlaybackState state);
    void durationChanged(ccos::core::Time duration);
    void audioLevelChanged(float left, float right);
    void markerReached(ccos::core::Time time, const QString& label);
    void playbackFinished();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    // Rendering pipeline
    void renderFrame();
    void uploadTexture(const QByteArray& data, int width, int height);
    void renderVideoFrame();
    void renderOverlays();
    void renderSafeZones();
    void renderFocusPeaking();
    void renderHistogram();
    void renderTimecode();
    void renderHUD();

    // Audio handling
    void initializeAudio();
    void startAudioPlayback();
    void stopAudioPlayback();
    void updateAudioLevels();

    // Frame timing
    void scheduleNextFrame();
    void calculateFrameTiming();
    qint64 getFrameTimestamp() const;

    // Helpers
    void updateState(PlaybackState newState);
    void notifyMarkerIfCrossed(ccos::core::Time oldTime, ccos::core::Time newTime);
    QRectF fitRectKeepAspect(const QRectF& source, const QRectF& target);

    // State
    timeline::Timeline* timeline_ = nullptr;
    PreviewConfig config_;
    PlaybackState state_ = PlaybackState::Stopped;
    Quality currentQuality_ = Quality::High;
    
    ccos::core::Time currentTime_;
    ccos::core::Time duration_;
    ccos::core::Time lastMarkerTime_;
    
    std::vector<std::pair<ccos::core::Time, QString>> markers_;
    
    bool muted_ = false;
    bool needRefresh_ = false;
    bool showSafeZones_ = false;
    bool showOverlays_ = true;
    bool showFocusPeaking_ = false;
    bool showHistogram_ = false;
    
    // OpenGL resources
    QOpenGLShaderProgram* shaderProgram_ = nullptr;
    QOpenGLBuffer vertexBuffer_;
    QOpenGLTexture* videoTexture_ = nullptr;
    
    // Audio
    std::unique_ptr<QAudioOutput> audioOutput_;
    std::unique_ptr<QIODevice> audioDevice_;
    float audioLevelLeft_ = 0.0f;
    float audioLevelRight_ = 0.0f;
    
    // Timing
    QTimer* frameTimer_ = nullptr;
    qint64 lastFrameTime_ = 0;
    qint64 frameInterval_ = 33; // ~30 FPS
    int droppedFrames_ = 0;
    
    // Mutex para thread safety
    mutable QMutex mutex_;
    
    // Pipeline de render
    std::shared_ptr<preview::RenderPipeline> renderPipeline_;
};

} // namespace ccos::ui
