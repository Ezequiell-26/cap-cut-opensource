/**
 * CCOS - Timeline UI Widget Implementation
 * Widget interactivo de timeline con OpenGL para edición profesional
 * Licencia: MIT
 */

#include "TimelineWidget.hpp"
#include <QPainter>
#include <QFontMetrics>
#include <QClipboard>
#include <QApplication>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace ccos::ui {

// ============================================================================
// Constructor y Destructor
// ============================================================================

TimelineWidget::TimelineWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , playbackTimer_(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // Configurar timer de playback (60 FPS)
    playbackTimer_->setInterval(16);
    connect(playbackTimer_, &QTimer::timeout, this, [this]() {
        if (isPlaying_) {
            auto fps = ccos::core::Time::fromFPS(30);
            playheadTime_ = playheadTime_ + fps;
            emit playheadTimeChanged(playheadTime_);
            update();
        }
    });
    
    // Inicializar valores por defecto
    workAreaStart_ = ccos::core::Time::zero();
    workAreaEnd_ = ccos::core::Time::fromSeconds(300); // 5 minutos por defecto
    
    vertexBuffer_.create();
    indexBuffer_.create();
}

TimelineWidget::~TimelineWidget() {
    makeCurrent();
    delete shaderProgram_;
    doneCurrent();
}

// ============================================================================
// Timeline Data Binding
// ============================================================================

void TimelineWidget::setTimeline(timeline::Timeline* timeline) {
    timeline_ = timeline;
    update();
}

// ============================================================================
// View Configuration
// ============================================================================

void TimelineWidget::setConfig(const TimelineViewConfig& config) {
    config_ = config;
    update();
}

void TimelineWidget::setZoomLevel(double level) {
    config_.zoomLevel = qBound(0.1, level, 10.0);
    config_.pixelsPerSecond = 50.0 * config_.zoomLevel;
    emit zoomChanged(config_.zoomLevel);
    update();
}

void TimelineWidget::setPixelsPerSecond(double pps) {
    config_.pixelsPerSecond = qMax(10.0, pps);
    update();
}

void TimelineWidget::fitToContents() {
    if (!timeline_) return;
    
    ccos::core::Time maxTime = ccos::core::Time::zero();
    for (const auto& track : timeline_->tracks()) {
        for (const auto& clip : track.clips()) {
            auto end = clip.start() + clip.duration();
            if (end > maxTime) maxTime = end;
        }
    }
    
    workAreaEnd_ = maxTime + ccos::core::Time::fromSeconds(5);
    
    double totalWidth = timeToX(workAreaEnd_);
    double widgetWidth = width() - config_.headerWidth;
    if (totalWidth > widgetWidth) {
        config_.pixelsPerSecond = widgetWidth / workAreaEnd_.seconds();
    }
    
    update();
}

void TimelineWidget::scrollToTime(ccos::core::Time time) {
    double x = timeToX(time);
    double widgetWidth = width() - config_.headerWidth;
    
    if (x < scrollOffset_) {
        scrollOffset_ = static_cast<int>(x - 20);
    } else if (x > scrollOffset_ + widgetWidth) {
        scrollOffset_ = static_cast<int>(x - widgetWidth + 20);
    }
    
    update();
}

// ============================================================================
// Playback Control
// ============================================================================

void TimelineWidget::setPlayheadTime(ccos::core::Time time) {
    playheadTime_ = qMax(ccos::core::Time::zero(), time);
    emit playheadTimeChanged(playheadTime_);
    update();
}

void TimelineWidget::setPlaying(bool playing) {
    isPlaying_ = playing;
    if (playing) {
        lastFrameTime_ = QDateTime::currentMSecsSinceEpoch();
        playbackTimer_->start();
    } else {
        playbackTimer_->stop();
    }
    emit playStateChanged(playing);
}

// ============================================================================
// Tools
// ============================================================================

void TimelineWidget::setCurrentTool(TimelineTool tool) {
    currentTool_ = tool;
    update();
}

// ============================================================================
// Selection
// ============================================================================

void TimelineWidget::clearSelection() {
    if (!selectedClips_.empty()) {
        selectedClips_.clear();
        emit selectionChanged();
        update();
    }
}

void TimelineWidget::selectClip(int trackIndex, int clipIndex) {
    auto pair = std::make_pair(trackIndex, clipIndex);
    if (std::find(selectedClips_.begin(), selectedClips_.end(), pair) == selectedClips_.end()) {
        selectedClips_.push_back(pair);
        emit selectionChanged();
        update();
    }
}

void TimelineWidget::deselectClip(int trackIndex, int clipIndex) {
    auto pair = std::make_pair(trackIndex, clipIndex);
    auto it = std::find(selectedClips_.begin(), selectedClips_.end(), pair);
    if (it != selectedClips_.end()) {
        selectedClips_.erase(it);
        emit selectionChanged();
        update();
    }
}

// ============================================================================
// Time Conversion Utilities
// ============================================================================

double TimelineWidget::timeToX(ccos::core::Time time) const {
    return config_.headerWidth + time.seconds() * config_.pixelsPerSecond - scrollOffset_;
}

ccos::core::Time TimelineWidget::xToTime(double x) const {
    double seconds = (x - config_.headerWidth + scrollOffset_) / config_.pixelsPerSecond;
    return ccos::core::Time::fromSeconds(seconds);
}

int TimelineWidget::trackToY(int trackIndex) const {
    return config_.rulerHeight + trackIndex * config_.trackHeight;
}

int TimelineWidget::yToTrack(int y) const {
    if (y < config_.rulerHeight) return -1;
    return (y - config_.rulerHeight) / config_.trackHeight;
}

// ============================================================================
// Clipboard Operations
// ============================================================================

void TimelineWidget::copySelected() {
    // Implementar copia al portapapeles
}

void TimelineWidget::cutSelected() {
    copySelected();
    deleteSelected();
}

void TimelineWidget::paste() {
    // Implementar pegado desde portapapeles
}

void TimelineWidget::deleteSelected() {
    if (!timeline_) return;
    
    auto& tracks = timeline_->tracks();
    // Ordenar en orden inverso para eliminar sin afectar índices
    std::sort(selectedClips_.begin(), selectedClips_.end(), std::greater<>());
    
    for (const auto& [trackIdx, clipIdx] : selectedClips_) {
        if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size())) {
            auto& track = tracks[trackIdx];
            auto& clips = track.clips();
            if (clipIdx >= 0 && clipIdx < static_cast<int>(clips.size())) {
                clips.erase(clips.begin() + clipIdx);
            }
        }
    }
    
    selectedClips_.clear();
    emit selectionChanged();
    update();
}

void TimelineWidget::rippleDeleteSelected() {
    deleteSelected();
    // TODO: Implementar ripple delete (desplazar clips posteriores)
}

// ============================================================================
// OpenGL Rendering
// ============================================================================

void TimelineWidget::initializeGL() {
    initializeOpenGLFunctions();
    createShaders();
    createBuffers();
    
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void TimelineWidget::createShaders() {
    shaderProgram_ = new QOpenGLShaderProgram(this);
    
    // Vertex shader
    const char* vertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec2 vertexPosition;
        layout(location = 1) in vec4 vertexColor;
        uniform mat4 projectionMatrix;
        out vec4 fragmentColor;
        void main() {
            gl_Position = projectionMatrix * vec4(vertexPosition, 0.0, 1.0);
            fragmentColor = vertexColor;
        }
    )";
    
    // Fragment shader
    const char* fragmentShaderSource = R"(
        #version 450 core
        in vec4 fragmentColor;
        out vec4 finalColor;
        void main() {
            finalColor = fragmentColor;
        }
    )";
    
    shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    shaderProgram_->link();
}

void TimelineWidget::createBuffers() {
    vao_.create();
    vao_.bind();
    
    vertexBuffer_.bind();
    indexBuffer_.bind();
    
    vao_.release();
    vertexBuffer_.release();
    indexBuffer_.release();
}

void TimelineWidget::loadTextures() {
    // TODO: Cargar texturas para waveforms y thumbnails
}

void TimelineWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    renderBackground();
    renderGrid();
    renderRuler();
    renderTrackHeaders();
    renderTracks();
    renderWorkArea();
    renderMarkers();
    renderPlayhead();
    renderKeyframes();
    renderToolOverlay();
    
    if (renderDragGhost_) {
        renderDragGhost();
    }
}

void TimelineWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    
    // Actualizar matriz de proyección
    shaderProgram_->bind();
    float left = 0.0f;
    float right = static_cast<float>(w);
    float bottom = static_cast<float>(h);
    float top = 0.0f;
    QMatrix4x4 projection;
    projection.ortho(left, right, bottom, top, -1.0f, 1.0f);
    shaderProgram_->setUniformValue("projectionMatrix", projection);
    shaderProgram_->release();
    
    horizontalScrollMax_ = static_cast<int>(timeToX(workAreaEnd_)) - w + config_.headerWidth;
    updateGeometry();
}

// ============================================================================
// Rendering Methods
// ============================================================================

void TimelineWidget::renderBackground() {
    shaderProgram_->bind();
    
    // Fondo principal
    QVector<float> vertices = {
        // Position        // Color
        0.0f, 0.0f,         0.12f, 0.12f, 0.12f, 1.0f,
        static_cast<float>(width()), 0.0f,  0.12f, 0.12f, 0.12f, 1.0f,
        static_cast<float>(width()), static_cast<float>(height()), 0.12f, 0.12f, 0.12f, 1.0f,
        0.0f, static_cast<float>(height()),         0.12f, 0.12f, 0.12f, 1.0f,
    };
    
    vertexBuffer_.bind();
    vertexBuffer_.allocate(vertices.data(), vertices.size() * sizeof(float));
    
    shaderProgram_->setAttributeBuffer(0, GL_FLOAT, 0, 2, 6 * sizeof(float));
    shaderProgram_->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 4, 6 * sizeof(float));
    shaderProgram_->enableAttributeArray(0);
    shaderProgram_->enableAttributeArray(1);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    shaderProgram_->release();
    vertexBuffer_.release();
}

void TimelineWidget::renderGrid() {
    shaderProgram_->bind();
    
    QVector<float> vertices;
    auto gridColor = config_.gridColor;
    float r = gridColor.redF();
    float g = gridColor.greenF();
    float b = gridColor.blueF();
    float a = 0.3f;
    
    // Líneas verticales cada segundo
    double startTime = xToTime(0).seconds();
    double endTime = xToTime(width()).seconds();
    
    for (double t = std::ceil(startTime); t <= endTime; t += 1.0) {
        double x = timeToX(ccos::core::Time::fromSeconds(t));
        vertices.append({static_cast<float>(x), 0.0f, r, g, b, a});
        vertices.append({static_cast<float>(x), static_cast<float>(height()), r, g, b, a});
    }
    
    // Líneas horizontales entre tracks
    int numTracks = timeline_ ? static_cast<int>(timeline_->tracks().size()) : 5;
    for (int i = 0; i <= numTracks; ++i) {
        int y = trackToY(i);
        vertices.append({static_cast<float>(config_.headerWidth), static_cast<float>(y), r, g, b, a});
        vertices.append({static_cast<float>(width()), static_cast<float>(y), r, g, b, a});
    }
    
    if (!vertices.empty()) {
        vertexBuffer_.bind();
        vertexBuffer_.allocate(vertices.data(), vertices.size() * sizeof(float));
        
        shaderProgram_->setAttributeBuffer(0, GL_FLOAT, 0, 2, 6 * sizeof(float));
        shaderProgram_->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 4, 6 * sizeof(float));
        shaderProgram_->enableAttributeArray(0);
        shaderProgram_->enableAttributeArray(1);
        
        glDrawArrays(GL_LINES, 0, vertices.size() / 6);
        
        vertexBuffer_.release();
    }
    
    shaderProgram_->release();
}

void TimelineWidget::renderRuler() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect rulerRect(config_.headerWidth, 0, width() - config_.headerWidth, config_.rulerHeight);
    painter.fillRect(rulerRect, QColor(35, 35, 35));
    
    painter.setFont(config_.timeFont);
    painter.setPen(config_.textColor);
    
    QFontMetrics fm(config_.timeFont);
    
    double startTime = xToTime(0).seconds();
    double endTime = xToTime(width()).seconds();
    
    // Determinar intervalo basado en zoom
    double interval = 1.0;
    if (config_.pixelsPerSecond < 20) interval = 10.0;
    else if (config_.pixelsPerSecond < 50) interval = 5.0;
    else if (config_.pixelsPerSecond > 200) interval = 0.5;
    
    for (double t = std::ceil(startTime / interval) * interval; t <= endTime; t += interval) {
        double x = timeToX(ccos::core::Time::fromSeconds(t));
        
        // Línea mayor
        painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), config_.rulerHeight - 5);
        
        // Texto del tiempo
        QString timeStr = QString("%1:%2")
            .arg(static_cast<int>(t) / 60, 2, 10, QChar('0'))
            .arg(static_cast<int>(t) % 60, 2, 10, QChar('0'));
        
        int textWidth = fm.horizontalAdvance(timeStr);
        painter.drawText(static_cast<int>(x) - textWidth / 2, config_.rulerHeight - 8, timeStr);
        
        // Líneas menores
        if (interval >= 1.0 && config_.pixelsPerSecond > 30) {
            for (int sub = 1; sub < interval; ++sub) {
                double subX = timeToX(ccos::core::Time::fromSeconds(t + sub));
                painter.drawLine(static_cast<int>(subX), config_.rulerHeight - 10, 
                               static_cast<int>(subX), config_.rulerHeight - 5);
            }
        }
    }
    
    // Header area
    painter.fillRect(0, 0, config_.headerWidth, config_.rulerHeight, QColor(25, 25, 25));
    painter.drawText(0, 0, config_.headerWidth, config_.rulerHeight, 
                    Qt::AlignCenter, "Tracks");
}

void TimelineWidget::renderTrackHeaders() {
    if (!timeline_) return;
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(config_.trackFont);
    painter.setPen(config_.textColor);
    
    const auto& tracks = timeline_->tracks();
    for (size_t i = 0; i < tracks.size(); ++i) {
        int y = trackToY(static_cast<int>(i));
        QRect headerRect(0, y, config_.headerWidth, config_.trackHeight);
        
        // Fondo del header
        painter.fillRect(headerRect, QColor(35, 35, 35));
        
        // Borde
        painter.setPen(QColor(60, 60, 60));
        painter.drawRect(headerRect);
        
        // Nombre del track
        painter.setPen(config_.textColor);
        QString trackName = QString("Track %1").arg(i + 1);
        // TODO: Usar nombre real del track cuando esté disponible
        painter.drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, 
                        " " + trackName);
    }
}

void TimelineWidget::renderTracks() {
    if (!timeline_) return;
    
    const auto& tracks = timeline_->tracks();
    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
        const auto& track = tracks[trackIdx];
        const auto& clips = track.clips();
        
        for (size_t clipIdx = 0; clipIdx < clips.size(); ++clipIdx) {
            bool isSelected = std::find(selectedClips_.begin(), selectedClips_.end(),
                                       std::make_pair(static_cast<int>(trackIdx), 
                                                     static_cast<int>(clipIdx))) 
                            != selectedClips_.end();
            renderClip(clips[clipIdx], static_cast<int>(trackIdx), static_cast<int>(clipIdx), isSelected);
        }
    }
}

void TimelineWidget::renderClip(const timeline::Clip& clip, int trackIndex, int clipIndex, bool isSelected) {
    QRectF rect = clipRect(clip, trackIndex);
    
    // Verificar si el clip está visible
    if (rect.right() < config_.headerWidth || rect.left() > width()) {
        return;
    }
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Color del clip
    QColor clipColor = isSelected ? config_.selectedClipColor : config_.clipColor;
    
    // Dibujar cuerpo del clip
    painter.fillRect(rect, clipColor);
    
    // Borde
    painter.setPen(isSelected ? QPen(QColor(200, 200, 200), 2) : QPen(QColor(80, 80, 80), 1));
    painter.drawRect(rect);
    
    // Handles de trim (izquierda y derecha)
    int handleWidth = 8;
    QColor handleColor(255, 255, 255, 200);
    
    // Handle izquierdo
    QRectF leftHandle(rect.left(), rect.top(), handleWidth, rect.height());
    painter.fillRect(leftHandle, handleColor);
    
    // Handle derecho
    QRectF rightHandle(rect.right() - handleWidth, rect.top(), handleWidth, rect.height());
    painter.fillRect(rightHandle, handleColor);
    
    // Waveform (si está habilitado y es audio)
    if (config_.showWaveforms && rect.width() > 30) {
        QRectF wfRect = waveformRect(clip, trackIndex);
        renderWaveform(clip, wfRect);
    }
    
    // Thumbnail (si está habilitado y es video)
    if (config_.showThumbnails && rect.width() > 50) {
        QRectF thumbRect = thumbnailRect(clip, trackIndex);
        renderThumbnail(clip, thumbRect);
    }
    
    // Nombre del clip
    painter.setFont(QFont("Segoe UI", 8));
    painter.setPen(QColor(255, 255, 255));
    QString clipName = QString("Clip %1").arg(clipIndex + 1);
    // TODO: Usar nombre real del clip
    painter.drawText(rect.adjusted(10, 5, -10, -5), Qt::AlignLeft | Qt::AlignTop, clipName);
    
    // Duración
    QString durationStr = QString::number(clip.duration().milliseconds() / 1000.0, 'f', 1) + "s";
    painter.drawText(rect.adjusted(10, 5, -10, -5), Qt::AlignRight | Qt::AlignTop, durationStr);
}

void TimelineWidget::renderWaveform(const timeline::Clip& clip, QRectF rect) {
    // TODO: Implementar renderizado de waveform real desde datos de audio
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.setPen(QPen(QColor(0, 0, 0, 100), 1));
    painter.drawLine(rect.topLeft(), rect.bottomLeft());
    painter.drawLine(rect.topRight(), rect.bottomRight());
    
    // Waveform simulado (línea central)
    painter.setPen(QPen(QColor(0, 0, 0, 150), 1));
    painter.drawLine(rect.topLeft(), rect.topRight());
}

void TimelineWidget::renderThumbnail(const timeline::Clip& clip, QRectF rect) {
    // TODO: Implementar renderizado de thumbnail real desde cache de video
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Thumbnail simulado (rectángulo gris)
    painter.fillRect(rect, QColor(0, 0, 0, 50));
    painter.setPen(QColor(0, 0, 0, 100));
    painter.drawRect(rect);
}

void TimelineWidget::renderPlayhead() {
    double x = timeToX(playheadTime_);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Línea del playhead
    QPen pen(config_.playheadColor, 2);
    painter.setPen(pen);
    painter.drawLine(static_cast<int>(x), config_.rulerHeight, 
                    static_cast<int>(x), height());
    
    // Cabeza del playhead (triángulo en la parte superior)
    QPoint headPoints[] = {
        QPoint(static_cast<int>(x) - 6, config_.rulerHeight),
        QPoint(static_cast<int>(x) + 6, config_.rulerHeight),
        QPoint(static_cast<int>(x), config_.rulerHeight + 10)
    };
    
    painter.setBrush(config_.playheadColor);
    painter.drawPolygon(headPoints, 3);
    
    // Tiempo actual
    QString timeStr = QString("%1:%2:%3")
        .arg(playheadTime_.hours(), 2, 10, QChar('0'))
        .arg(playheadTime_.minutes() % 60, 2, 10, QChar('0'))
        .arg(playheadTime_.seconds() % 60, 2, 10, QChar('0'));
    
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    int textWidth = painter.fontMetrics().horizontalAdvance(timeStr);
    painter.fillRect(static_cast<int>(x) - textWidth / 2 - 4, 2, 
                    textWidth + 8, 18, QColor(0, 0, 0, 200));
    painter.setPen(QColor(255, 255, 255));
    painter.drawText(static_cast<int>(x) - textWidth / 2, 16, timeStr);
}

void TimelineWidget::renderWorkArea() {
    double startX = timeToX(workAreaStart_);
    double endX = timeToX(workAreaEnd_);
    
    QPainter painter(this);
    
    // Área fuera del work area (atenuada)
    if (startX > config_.headerWidth) {
        painter.fillRect(config_.headerWidth, config_.rulerHeight, 
                        static_cast<int>(startX) - config_.headerWidth, height(),
                        QColor(0, 0, 0, 100));
    }
    
    if (endX < width()) {
        painter.fillRect(static_cast<int>(endX), config_.rulerHeight,
                        width() - static_cast<int>(endX), height(),
                        QColor(0, 0, 0, 100));
    }
}

void TimelineWidget::renderMarkers() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    for (const auto& marker : markers_) {
        double x = timeToX(marker);
        
        // Línea del marcador
        painter.setPen(QPen(QColor(255, 200, 0), 1, Qt::DashLine));
        painter.drawLine(static_cast<int>(x), config_.rulerHeight, 
                        static_cast<int>(x), height());
        
        // Triángulo del marcador
        QPoint markerPoints[] = {
            QPoint(static_cast<int>(x) - 5, 0),
            QPoint(static_cast<int>(x) + 5, 0),
            QPoint(static_cast<int>(x), 8)
        };
        painter.setBrush(QColor(255, 200, 0));
        painter.drawPolygon(markerPoints, 3);
    }
}

void TimelineWidget::renderKeyframes() {
    if (!config_.showKeyframes || !timeline_) return;
    
    // TODO: Implementar renderizado de keyframes
}

void TimelineWidget::renderToolOverlay() {
    // Mostrar indicador de herramienta actual
    QString toolName;
    switch (currentTool_) {
        case TimelineTool::Select: toolName = "Select (V)"; break;
        case TimelineTool::Razor: toolName = "Razor (C)"; break;
        case TimelineTool::Trim: toolName = "Trim (T)"; break;
        case TimelineTool::Slip: toolName = "Slip (S)"; break;
        case TimelineTool::Slide: toolName = "Slide (D)"; break;
        case TimelineTool::Pen: toolName = "Pen (P)"; break;
        case TimelineTool::Hand: toolName = "Hand (H)"; break;
        case TimelineTool::Zoom: toolName = "Zoom (Z)"; break;
    }
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(QFont("Segoe UI", 8));
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(10, height() - 10, toolName);
}

void TimelineWidget::renderDragGhost() {
    if (!renderDragGhost_) return;
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fantasma semi-transparente
    painter.setOpacity(0.5);
    painter.fillRect(dragGhostRect_, QColor(100, 180, 255, 128));
    painter.setPen(QPen(QColor(200, 200, 200), 2, Qt::DashLine));
    painter.drawRect(dragGhostRect_);
    
    // Etiqueta
    if (!dragGhostLabel_.isEmpty()) {
        painter.setOpacity(1.0);
        painter.setPen(QColor(255, 255, 255));
        painter.drawText(dragGhostRect_.adjusted(5, 5, -5, -5), 
                        Qt::AlignLeft | Qt::AlignTop, dragGhostLabel_);
    }
}

// ============================================================================
// Interaction Handlers
// ============================================================================

SnapPoint TimelineWidget::findSnapPoint(ccos::core::Time time, int trackIndex, int clipIndex) {
    SnapPoint result{ccos::core::Time::zero(), SnapTarget::None};
    
    if (!config_.snapEnabled || !timeline_) return result;
    
    double timeX = timeToX(time);
    double threshold = config_.snapThreshold;
    double closestDist = threshold;
    
    const auto& tracks = timeline_->tracks();
    
    // Verificar clips en todos los tracks
    for (int t = 0; t < static_cast<int>(tracks.size()); ++t) {
        const auto& clips = tracks[t].clips();
        for (int c = 0; c < static_cast<int>(clips.size()); ++c) {
            if (t == trackIndex && c == clipIndex) continue; // Ignorar el clip actual
            
            const auto& clip = clips[c];
            
            // Inicio del clip
            double startX = timeToX(clip.start());
            double distStart = std::abs(startX - timeX);
            if (distStart < closestDist) {
                closestDist = distStart;
                result = {clip.start(), SnapTarget::ClipStart, t, c, "Clip Start"};
            }
            
            // Fin del clip
            double endX = timeToX(clip.start() + clip.duration());
            double distEnd = std::abs(endX - timeX);
            if (distEnd < closestDist) {
                closestDist = distEnd;
                result = {clip.start() + clip.duration(), SnapTarget::ClipEnd, t, c, "Clip End"};
            }
        }
    }
    
    // Playhead
    double playheadX = timeToX(playheadTime_);
    double distPlayhead = std::abs(playheadX - timeX);
    if (distPlayhead < closestDist) {
        result = {playheadTime_, SnapTarget::Playhead, -1, -1, "Playhead"};
    }
    
    // Marcadores
    for (const auto& marker : markers_) {
        double markerX = timeToX(marker);
        double distMarker = std::abs(markerX - timeX);
        if (distMarker < closestDist) {
            result = {marker, SnapTarget::Marker, -1, -1, "Marker"};
        }
    }
    
    return result;
}

ccos::core::Time TimelineWidget::snapToGrid(ccos::core::Time time) {
    if (!config_.snapEnabled) return time;
    
    // Snap a frames (30 FPS por defecto)
    auto frameDuration = ccos::core::Time::fromFPS(30);
    qint64 frames = qRound64(time.milliseconds() / frameDuration.milliseconds());
    return frameDuration * frames;
}

void TimelineWidget::startDrag(QPoint pos, int trackIndex, int clipIndex) {
    if (!timeline_) return;
    
    const auto& tracks = timeline_->tracks();
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    
    const auto& clips = tracks[trackIndex].clips();
    if (clipIndex < 0 || clipIndex >= static_cast<int>(clips.size())) return;
    
    dragState_.isDragging = true;
    dragState_.draggedTrack = trackIndex;
    dragState_.draggedClip = clipIndex;
    dragState_.startPos = pos;
    
    const auto& clip = clips[clipIndex];
    dragState_.originalStartTime = clip.start();
    dragState_.originalDuration = clip.duration();
    dragState_.originalSourceIn = clip.sourceIn();
    
    renderDragGhost_ = true;
    dragGhostRect_ = clipRect(clip, trackIndex);
    dragGhostLabel_ = QString("Clip %1").arg(clipIndex + 1);
}

void TimelineWidget::updateDrag(QPoint pos) {
    if (!dragState_.isDragging || !timeline_) return;
    
    int dx = pos.x() - dragState_.startPos.x();
    double deltaSeconds = dx / config_.pixelsPerSecond;
    
    auto newStart = dragState_.originalStartTime + ccos::core::Time::fromSeconds(deltaSeconds);
    newStart = snapToGrid(newStart);
    
    // Aplicar snapping
    if (config_.snapEnabled) {
        auto snap = findSnapPoint(newStart, dragState_.draggedTrack, dragState_.draggedClip);
        if (snap.type != SnapTarget::None) {
            newStart = snap.time;
        }
    }
    
    // Actualizar fantasma
    auto& tracks = timeline_->tracks();
    const auto& clips = tracks[dragState_.draggedTrack].clips();
    const auto& clip = clips[dragState_.draggedClip];
    
    QRectF ghostRect = dragGhostRect_;
    ghostRect.moveLeft(timeToX(newStart));
    dragGhostRect_ = ghostRect;
    
    update();
}

void TimelineWidget::endDrag() {
    if (!dragState_.isDragging || !timeline_) {
        dragState_.isDragging = false;
        renderDragGhost_ = false;
        return;
    }
    
    int dx = QCursor::pos().x() - dragState_.startPos.x();
    double deltaSeconds = dx / config_.pixelsPerSecond;
    
    auto newStart = dragState_.originalStartTime + ccos::core::Time::fromSeconds(deltaSeconds);
    newStart = snapToGrid(newStart);
    
    // Aplicar snapping final
    if (config_.snapEnabled) {
        auto snap = findSnapPoint(newStart, dragState_.draggedTrack, dragState_.draggedClip);
        if (snap.type != SnapTarget::None) {
            newStart = snap.time;
        }
    }
    
    // Actualizar clip en timeline
    auto& tracks = timeline_->tracks();
    auto& clips = tracks[dragState_.draggedTrack].clips();
    clips[dragState_.draggedClip].setStart(newStart);
    
    emit clipMoved(dragState_.draggedTrack, dragState_.draggedClip, newStart);
    
    dragState_.isDragging = false;
    renderDragGhost_ = false;
    update();
}

void TimelineWidget::splitClipAtPlayhead() {
    if (!timeline_) return;
    
    const auto& tracks = timeline_->tracks();
    for (int trackIdx = 0; trackIdx < static_cast<int>(tracks.size()); ++trackIdx) {
        auto& clips = tracks[trackIdx].clips();
        for (int clipIdx = 0; clipIdx < static_cast<int>(clips.size()); ++clipIdx) {
            auto& clip = clips[clipIdx];
            if (playheadTime_ > clip.start() && playheadTime_ < clip.start() + clip.duration()) {
                emit clipSplit(trackIdx, playheadTime_);
                // TODO: Implementar split real del clip
                break;
            }
        }
    }
}

void TimelineWidget::razorClipAtPosition(QPoint pos) {
    if (!timeline_) return;
    
    int trackIndex = yToTrack(pos.y());
    if (trackIndex < 0) return;
    
    ccos::core::Time time = xToTime(pos.x());
    
    const auto& clips = timeline_->tracks()[trackIndex].clips();
    for (size_t clipIdx = 0; clipIdx < clips.size(); ++clipIdx) {
        const auto& clip = clips[clipIdx];
        if (time > clip.start() && time < clip.start() + clip.duration()) {
            emit clipSplit(trackIndex, time);
            // TODO: Implementar corte real del clip
            break;
        }
    }
}

// ============================================================================
// Event Handlers
// ============================================================================

void TimelineWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        
        // Verificar si clic en área de tracks
        int trackIndex = yToTrack(pos.y());
        if (trackIndex >= 0 && timeline_) {
            const auto& clips = timeline_->tracks()[trackIndex].clips();
            
            // Buscar clip bajo el cursor
            int clickedClip = -1;
            for (size_t i = 0; i < clips.size(); ++i) {
                QRectF clipR = clipRect(clips[i], trackIndex);
                if (clipR.contains(pos)) {
                    clickedClip = static_cast<int>(i);
                    
                    // Verificar si clic en handles de trim
                    if (pos.x() < clipR.left() + 8) {
                        dragState_.isTrimming = true;
                        dragState_.trimEdge = TrimEdge::Left;
                    } else if (pos.x() > clipR.right() - 8) {
                        dragState_.isTrimming = true;
                        dragState_.trimEdge = TrimEdge::Right;
                    }
                    break;
                }
            }
            
            if (clickedClip >= 0) {
                if (event->modifiers() & Qt::ControlModifier) {
                    selectClip(trackIndex, clickedClip);
                } else {
                    clearSelection();
                    selectClip(trackIndex, clickedClip);
                }
                
                if (!dragState_.isTrimming) {
                    startDrag(pos, trackIndex, clickedClip);
                }
            } else {
                clearSelection();
            }
        } else if (pos.y() >= config_.rulerHeight) {
            // Clic en área vacía
            clearSelection();
        }
    } else if (event->button() == Qt::MiddleButton || 
              (event->button() == Qt::LeftButton && event->modifiers() & Qt::AltModifier)) {
        // Hand tool temporal
        dragState_.isDragging = true;
        dragState_.startPos = pos;
    }
    
    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    QPoint pos = event->pos();
    
    if (dragState_.isTrimming) {
        // Lógica de trim
        updateDrag(pos);
    } else if (dragState_.isDragging && !renderDragGhost_) {
        // Hand tool o scroll
        int dx = pos.x() - dragState_.startPos.x();
        scrollOffset_ = qBound(0, scrollOffset_ - dx, horizontalScrollMax_);
        dragState_.startPos = pos;
        update();
    } else if (dragState_.isDragging && renderDragGhost_) {
        updateDrag(pos);
    }
    
    // Actualizar cursor según contexto
    if (!dragState_.isDragging && !dragState_.isTrimming) {
        int trackIndex = yToTrack(pos.y());
        if (trackIndex >= 0 && timeline_) {
            const auto& clips = timeline_->tracks()[trackIndex].clips();
            bool onEdge = false;
            for (const auto& clip : clips) {
                QRectF clipR = clipRect(clip, trackIndex);
                if (clipR.contains(pos)) {
                    if (pos.x() < clipR.left() + 8 || pos.x() > clipR.right() - 8) {
                        onEdge = true;
                        break;
                    }
                }
            }
            setCursor(onEdge ? Qt::SizeHorCursor : Qt::ArrowCursor);
        }
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (dragState_.isTrimming) {
            endDrag();
            dragState_.isTrimming = false;
        } else {
            endDrag();
        }
    }
    
    update();
}

void TimelineWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Doble clic para zoom fit o editar clip
        QPoint pos = event->pos();
        int trackIndex = yToTrack(pos.y());
        
        if (trackIndex >= 0 && timeline_) {
            const auto& clips = timeline_->tracks()[trackIndex].clips();
            for (size_t i = 0; i < clips.size(); ++i) {
                QRectF clipR = clipRect(clips[i], trackIndex);
                if (clipR.contains(pos)) {
                    // TODO: Abrir editor de clip
                    break;
                }
            }
        }
    }
}

void TimelineWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom con Ctrl+wheel
        double delta = event->angleDelta().y() / 120.0;
        double newZoom = config_.zoomLevel + delta * 0.2;
        setZoomLevel(newZoom);
    } else {
        // Scroll horizontal
        int delta = -event->angleDelta().x() / 8;
        if (delta == 0) delta = -event->angleDelta().y() / 8;
        scrollOffset_ = qBound(0, scrollOffset_ + delta, horizontalScrollMax_);
        update();
    }
}

void TimelineWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_V:
            setCurrentTool(TimelineTool::Select);
            break;
        case Qt::Key_C:
            setCurrentTool(TimelineTool::Razor);
            break;
        case Qt::Key_T:
            setCurrentTool(TimelineTool::Trim);
            break;
        case Qt::Key_S:
            setCurrentTool(TimelineTool::Slip);
            break;
        case Qt::Key_D:
            setCurrentTool(TimelineTool::Slide);
            break;
        case Qt::Key_P:
            setCurrentTool(TimelineTool::Pen);
            break;
        case Qt::Key_H:
            setCurrentTool(TimelineTool::Hand);
            break;
        case Qt::Key_Z:
            setCurrentTool(TimelineTool::Zoom);
            break;
        case Qt::Key_Space:
            setPlaying(!isPlaying_);
            break;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            if (event->modifiers() & Qt::ShiftModifier) {
                rippleDeleteSelected();
            } else {
                deleteSelected();
            }
            break;
        case Qt::Key_Left:
            if (event->modifiers() & Qt::ControlModifier) {
                setPlayheadTime(playheadTime_ - ccos::core::Time::fromFrames(1, 30));
            } else {
                setPlayheadTime(playheadTime_ - ccos::core::Time::fromSeconds(1));
            }
            break;
        case Qt::Key_Right:
            if (event->modifiers() & Qt::ControlModifier) {
                setPlayheadTime(playheadTime_ + ccos::core::Time::fromFrames(1, 30));
            } else {
                setPlayheadTime(playheadTime_ + ccos::core::Time::fromSeconds(1));
            }
            break;
        case Qt::Key_Home:
            setPlayheadTime(ccos::core::Time::zero());
            break;
        case Qt::Key_End:
            setPlayheadTime(workAreaEnd_);
            break;
    }
    
    QWidget::keyPressEvent(event);
}

void TimelineWidget::contextMenuEvent(QContextMenuEvent* event) {
    QPoint pos = event->pos();
    int trackIndex = yToTrack(pos.y());
    int clipIndex = -1;
    
    if (trackIndex >= 0 && timeline_) {
        const auto& clips = timeline_->tracks()[trackIndex].clips();
        for (size_t i = 0; i < clips.size(); ++i) {
            QRectF clipR = clipRect(clips[i], trackIndex);
            if (clipR.contains(pos)) {
                clipIndex = static_cast<int>(i);
                break;
            }
        }
    }
    
    emit contextMenuRequested(pos, trackIndex, clipIndex);
}

// ============================================================================
// Helpers
// ============================================================================

QRectF TimelineWidget::clipRect(const timeline::Clip& clip, int trackIndex) const {
    double x = timeToX(clip.start());
    double y = trackToY(trackIndex);
    double width = clip.duration().seconds() * config_.pixelsPerSecond;
    
    return QRectF(x, y + 5, width, config_.trackHeight - 10);
}

QRectF TimelineWidget::waveformRect(const timeline::Clip& clip, int trackIndex) const {
    QRectF clipR = clipRect(clip, trackIndex);
    return clipR.adjusted(0, clipR.height() / 3, 0, -clipR.height() / 3);
}

QRectF TimelineWidget::thumbnailRect(const timeline::Clip& clip, int trackIndex) const {
    QRectF clipR = clipRect(clip, trackIndex);
    return clipR.adjusted(5, 25, -5, -5);
}

void TimelineWidget::updateGeometry() {
    // Actualizar scrollbar si es necesario
    // TODO: Integrar con QScrollBar real
}

} // namespace ccos::ui
