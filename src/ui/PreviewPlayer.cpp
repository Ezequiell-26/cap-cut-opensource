/**
 * CCOS - Preview Player Widget Implementation
 * Reproductor de preview frame-accurate con sincronización audio/video
 * Licencia: MIT
 */

#include "PreviewPlayer.hpp"
#include <QPainter>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QtMath>
#include <chrono>
#include <algorithm>

namespace ccos::ui {

// ============================================================================
// Constructor y Destructor
// ============================================================================

PreviewPlayer::PreviewPlayer(QWidget* parent)
    : QOpenGLWidget(parent)
    , frameTimer_(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    
    // Configurar timer de frames
    connect(frameTimer_, &QTimer::timeout, this, [this]() {
        if (state_ == PlaybackState::Playing) {
            renderFrame();
        }
    });
    
    // Inicializar configuración por defecto
    currentTime_ = ccos::core::Time::zero();
    duration_ = ccos::core::Time::fromSeconds(300); // 5 minutos por defecto
    
    vertexBuffer_.create();
}

PreviewPlayer::~PreviewPlayer() {
    makeCurrent();
    delete shaderProgram_;
    delete videoTexture_;
    doneCurrent();
}

// ============================================================================
// Timeline Binding
// ============================================================================

void PreviewPlayer::setTimeline(timeline::Timeline* timeline) {
    QMutexLocker locker(&mutex_);
    timeline_ = timeline;
    
    // Calcular duración basada en clips
    if (timeline_) {
        ccos::core::Time maxTime = ccos::core::Time::zero();
        for (const auto& track : timeline_->tracks()) {
            for (const auto& clip : track.clips()) {
                auto end = clip.start() + clip.duration();
                if (end > maxTime) maxTime = end;
            }
        }
        duration_ = maxTime;
        emit durationChanged(duration_);
    }
    
    update();
}

// ============================================================================
// Playback Control
// ============================================================================

void PreviewPlayer::play() {
    if (state_ == PlaybackState::Playing) return;
    
    updateState(PlaybackState::Playing);
    
    if (currentTime_ >= duration_) {
        currentTime_ = ccos::core::Time::zero();
    }
    
    calculateFrameTiming();
    frameTimer_->start(frameInterval_);
    
    if (config_.enableAudio && !muted_) {
        startAudioPlayback();
    }
    
    emit playbackStateChanged(state_);
}

void PreviewPlayer::pause() {
    if (state_ != PlaybackState::Playing) return;
    
    updateState(PlaybackState::Paused);
    frameTimer_->stop();
    stopAudioPlayback();
    
    emit playbackStateChanged(state_);
}

void PreviewPlayer::stop() {
    updateState(PlaybackState::Stopped);
    frameTimer_->stop();
    stopAudioPlayback();
    currentTime_ = ccos::core::Time::zero();
    
    emit playbackStateChanged(state_);
    emit frameRendered(currentTime_);
    update();
}

void PreviewPlayer::seek(ccos::core::Time time) {
    QMutexLocker locker(&mutex_);
    
    auto oldTime = currentTime_;
    currentTime_ = qMax(ccos::core::Time::zero(), qMin(time, duration_));
    
    updateState(PlaybackState::Seeking);
    
    // Buscar frame más cercano
    needRefresh_ = true;
    
    if (config_.enableAudio) {
        stopAudioPlayback();
        if (state_ == PlaybackState::Playing) {
            startAudioPlayback();
        }
    }
    
    notifyMarkerIfCrossed(oldTime, currentTime_);
    
    updateState(state_ == PlaybackState::Playing ? PlaybackState::Playing : PlaybackState::Paused);
    
    emit frameRendered(currentTime_);
    update();
}

void PreviewPlayer::stepForward() {
    auto frameDuration = ccos::core::Time::fromFPS(config_.targetFPS);
    seek(currentTime_ + frameDuration);
}

void PreviewPlayer::stepBack() {
    auto frameDuration = ccos::core::Time::fromFPS(config_.targetFPS);
    seek(currentTime_ - frameDuration);
}

void PreviewPlayer::stepFrame(int frames) {
    auto frameDuration = ccos::core::Time::fromFPS(config_.targetFPS);
    seek(currentTime_ + frameDuration * frames);
}

// ============================================================================
// Configuration
// ============================================================================

void PreviewPlayer::setConfig(const PreviewConfig& config) {
    config_ = config;
    calculateFrameTiming();
    update();
}

void PreviewPlayer::setVolume(float volume) {
    config_.audioVolume = qBound(0.0f, volume, 1.0f);
    if (audioOutput_) {
        audioOutput_->setVolume(config_.audioVolume);
    }
}

void PreviewPlayer::setMuted(bool muted) {
    muted_ = muted;
    if (muted && audioOutput_) {
        audioOutput_->setVolume(0.0f);
    } else if (!muted && audioOutput_) {
        audioOutput_->setVolume(config_.audioVolume);
    }
}

void PreviewPlayer::setQuality(Quality quality) {
    currentQuality_ = quality;
    
    // Ajustar resolución objetivo según calidad
    switch (quality) {
        case Quality::Low:
            config_.targetWidth = 640;
            config_.targetHeight = 360;
            break;
        case Quality::Medium:
            config_.targetWidth = 960;
            config_.targetHeight = 540;
            break;
        case Quality::High:
            config_.targetWidth = 1920;
            config_.targetHeight = 1080;
            break;
        case Quality::Maximum:
            config_.targetWidth = 3840;
            config_.targetHeight = 2160;
            break;
    }
    
    needRefresh_ = true;
    update();
}

void PreviewPlayer::setShowSafeZones(bool show) {
    showSafeZones_ = show;
    update();
}

void PreviewPlayer::setShowOverlays(bool show) {
    showOverlays_ = show;
    update();
}

void PreviewPlayer::setShowFocusPeaking(bool show) {
    showFocusPeaking_ = show;
    update();
}

void PreviewPlayer::setShowHistogram(bool show) {
    showHistogram_ = show;
    update();
}

// ============================================================================
// Markers
// ============================================================================

void PreviewPlayer::addMarker(ccos::core::Time time, const QString& label) {
    markers_.emplace_back(time, label);
    std::sort(markers_.begin(), markers_.end(), 
              [](const auto& a, const auto& b) { return a.first < b.first; });
    update();
}

void PreviewPlayer::removeMarker(ccos::core::Time time) {
    markers_.erase(
        std::remove_if(markers_.begin(), markers_.end(),
                       [time](const auto& m) { return m.first == time; }),
        markers_.end());
    update();
}

void PreviewPlayer::clearMarkers() {
    markers_.clear();
    update();
}

// ============================================================================
// OpenGL Rendering
// ============================================================================

void PreviewPlayer::initializeGL() {
    initializeOpenGLFunctions();
    
    // Crear shader program
    shaderProgram_ = new QOpenGLShaderProgram(this);
    
    const char* vertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec2 vertexPosition;
        layout(location = 1) in vec2 texCoord;
        out vec2 fragTexCoord;
        uniform mat4 projectionMatrix;
        void main() {
            gl_Position = projectionMatrix * vec4(vertexPosition, 0.0, 1.0);
            fragTexCoord = texCoord;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 450 core
        in vec2 fragTexCoord;
        out vec4 finalColor;
        uniform sampler2D videoTexture;
        uniform float brightness;
        uniform float contrast;
        uniform float saturation;
        
        vec3 rgb2hsv(vec3 c) {
            vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
            vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
            vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
            float d = q.x - min(q.w, q.y);
            float e = 1.0e-10;
            return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
        }
        
        vec3 hsv2rgb(vec3 c) {
            vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
            vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
            return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }
        
        void main() {
            vec3 color = texture(videoTexture, fragTexCoord).rgb;
            
            // Ajuste de brillo
            color *= brightness;
            
            // Ajuste de contraste
            color = (color - 0.5) * contrast + 0.5;
            
            // Ajuste de saturación
            vec3 hsv = rgb2hsv(color);
            hsv.y *= saturation;
            color = hsv2rgb(hsv);
            
            finalColor = vec4(color, 1.0);
        }
    )";
    
    shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    shaderProgram_->link();
    
    // Crear textura de video
    videoTexture_ = new QOpenGLTexture(QOpenGLTexture::Target2D);
    videoTexture_->setFormat(QOpenGLTexture::RGBA8_UNorm);
    videoTexture_->setMinificationFilter(QOpenGLTexture::Linear);
    videoTexture_->setMagnificationFilter(QOpenGLTexture::Linear);
    videoTexture_->setWrapMode(QOpenGLTexture::ClampToEdge);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    initializeAudio();
}

void PreviewPlayer::initializeAudio() {
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    
    audioOutput_ = std::make_unique<QAudioOutput>(format);
    audioOutput_->setVolume(config_.audioVolume);
}

void PreviewPlayer::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    renderVideoFrame();
    
    if (showOverlays_) {
        renderOverlays();
    }
}

void PreviewPlayer::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    
    shaderProgram_->bind();
    QMatrix4x4 projection;
    projection.ortho(0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, -1.0f, 1.0f);
    shaderProgram_->setUniformValue("projectionMatrix", projection);
    shaderProgram_->release();
}

// ============================================================================
// Video Rendering
// ============================================================================

void PreviewPlayer::renderFrame() {
    QMutexLocker locker(&mutex_);
    
    if (!timeline_) return;
    
    // Calcular tiempo del frame
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    if (lastFrameTime_ > 0) {
        auto delta = elapsed - lastFrameTime_;
        if (delta > frameInterval_ * 2) {
            droppedFrames_++;
        }
    }
    lastFrameTime_ = elapsed;
    
    // Avanzar tiempo
    auto frameDuration = ccos::core::Time::fromFPS(config_.targetFPS);
    currentTime_ = currentTime_ + frameDuration;
    
    // Verificar loop
    if (currentTime_ >= duration_) {
        if (config_.loopEnabled) {
            currentTime_ = ccos::core::Time::zero();
        } else {
            stop();
            emit playbackFinished();
            return;
        }
    }
    
    // Verificar markers
    notifyMarkerIfCrossed(currentTime_ - frameDuration, currentTime_);
    
    // Renderizar frame actual
    needRefresh_ = true;
    update();
    
    emit frameRendered(currentTime_);
}

void PreviewPlayer::uploadTexture(const QByteArray& data, int width, int height) {
    makeCurrent();
    
    if (videoTexture_->isCreated()) {
        videoTexture_->destroy();
    }
    
    videoTexture_->create();
    videoTexture_->setSize(width, height);
    videoTexture_->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    videoTexture_->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, data.constData());
    
    doneCurrent();
}

void PreviewPlayer::renderVideoFrame() {
    if (!shaderProgram_ || !videoTexture_) return;
    
    shaderProgram_->bind();
    
    // Configurar vértices para fullscreen quad
    QRectF rect = fitRectKeepAspect(QRectF(0, 0, config_.targetWidth, config_.targetHeight),
                                   QRectF(0, 0, width(), height()));
    
    QVector<float> vertices = {
        // Position         // TexCoord
        rect.left(), rect.top(),      0.0f, 0.0f,
        rect.right(), rect.top(),     1.0f, 0.0f,
        rect.right(), rect.bottom(),  1.0f, 1.0f,
        rect.left(), rect.bottom(),   0.0f, 1.0f,
    };
    
    vertexBuffer_.bind();
    vertexBuffer_.allocate(vertices.data(), vertices.size() * sizeof(float));
    
    shaderProgram_->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
    shaderProgram_->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
    shaderProgram_->enableAttributeArray(0);
    shaderProgram_->enableAttributeArray(1);
    
    // Uniformes de color
    shaderProgram_->setUniformValue("brightness", 1.0f);
    shaderProgram_->setUniformValue("contrast", 1.0f);
    shaderProgram_->setUniformValue("saturation", 1.0f);
    
    // Bind textura
    videoTexture_->bind();
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    videoTexture_->release();
    vertexBuffer_.release();
    shaderProgram_->release();
}

void PreviewPlayer::renderOverlays() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (showSafeZones_) {
        renderSafeZones();
    }
    
    if (showFocusPeaking_) {
        renderFocusPeaking();
    }
    
    if (showHistogram_) {
        renderHistogram();
    }
    
    renderTimecode();
    renderHUD();
}

void PreviewPlayer::renderSafeZones() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF safeRect = rect().adjusted(50, 50, -50, -50);
    QRectF actionRect = rect().adjusted(25, 25, -25, -25);
    
    QPen pen(QColor(0, 255, 0, 150), 1, Qt::DashLine);
    painter.setPen(pen);
    painter.drawRect(safeRect);
    painter.drawRect(actionRect);
    
    // Etiquetas
    painter.setFont(QFont("Consolas", 8));
    painter.setPen(QColor(0, 255, 0));
    painter.drawText(55, 45, "Title Safe");
    painter.drawText(30, 20, "Action Safe");
}

void PreviewPlayer::renderFocusPeaking() {
    // TODO: Implementar focus peaking basado en detección de bordes
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Simulación visual
    QPen pen(QColor(255, 0, 0, 100), 2);
    painter.setPen(pen);
    
    // Dibujar bordes detectados (simulado)
    int cx = width() / 2;
    int cy = height() / 2;
    painter.drawEllipse(QPoint(cx, cy), 50, 50);
}

void PreviewPlayer::renderHistogram() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Histograma en esquina inferior derecha
    int histWidth = 200;
    int histHeight = 100;
    int histX = width() - histWidth - 10;
    int histY = height() - histHeight - 50;
    
    QRectF histRect(histX, histY, histWidth, histHeight);
    
    // Fondo semi-transparente
    painter.fillRect(histRect, QColor(0, 0, 0, 150));
    
    // Borde
    painter.setPen(QColor(100, 100, 100));
    painter.drawRect(histRect);
    
    // Histograma simulado (RGB)
    QVector<QColor> colors = {Qt::red, Qt::green, Qt::blue};
    for (int c = 0; c < 3; ++c) {
        painter.setPen(QPen(colors[c], 1));
        int baseY = histY + histHeight - 5;
        int prevX = histX;
        int prevH = histHeight / 3;
        
        for (int i = 0; i < histWidth; i += 5) {
            int h = qrand() % (histHeight / 2);
            painter.drawLine(prevX, baseY - prevH, histX + i, baseY - h);
            prevX = histX + i;
            prevH = h;
        }
    }
}

void PreviewPlayer::renderTimecode() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QString timeStr = QString("%1:%2:%3.%4")
        .arg(currentTime_.hours(), 2, 10, QChar('0'))
        .arg(currentTime_.minutes() % 60, 2, 10, QChar('0'))
        .arg(currentTime_.seconds() % 60, 2, 10, QChar('0'))
        .arg((currentTime_.milliseconds() % 1000) / 10, 2, 10, QChar('0'));
    
    painter.setFont(QFont("Consolas", 14, QFont::Bold));
    painter.setPen(QColor(255, 255, 255));
    
    // Fondo semi-transparente
    int textWidth = painter.fontMetrics().horizontalAdvance(timeStr);
    int textHeight = painter.fontMetrics().height();
    QRectF bgRect(10, height() - textHeight - 20, textWidth + 20, textHeight + 10);
    painter.fillRect(bgRect, QColor(0, 0, 0, 180));
    
    painter.drawText(bgRect.adjusted(10, 5, -10, -5), Qt::AlignLeft | Qt::AlignVCenter, timeStr);
}

void PreviewPlayer::renderHUD() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.setFont(QFont("Segoe UI", 9));
    painter.setPen(QColor(200, 200, 200));
    
    // Información de calidad
    QString qualityStr;
    switch (currentQuality_) {
        case Quality::Low: qualityStr = "LOW"; break;
        case Quality::Medium: qualityStr = "MED"; break;
        case Quality::High: qualityStr = "HIGH"; break;
        case Quality::Maximum: qualityStr = "MAX"; break;
    }
    
    QString info = QString("%1x%2 @ %3 FPS [%4]")
        .arg(config_.targetWidth)
        .arg(config_.targetHeight)
        .arg(config_.targetFPS, 0, 'f', 0)
        .arg(qualityStr);
    
    painter.drawText(width() - painter.fontMetrics().horizontalAdvance(info) - 10, 20, info);
    
    // Estado de reproducción
    QString stateStr;
    switch (state_) {
        case PlaybackState::Stopped: stateStr = "STOPPED"; break;
        case PlaybackState::Playing: stateStr = "PLAYING"; break;
        case PlaybackState::Paused: stateStr = "PAUSED"; break;
        case PlaybackState::Seeking: stateStr = "SEEKING"; break;
        case PlaybackState::Buffering: stateStr = "BUFFERING"; break;
    }
    
    painter.drawText(10, 20, stateStr);
    
    // Contador de frames dropped
    if (droppedFrames_ > 0) {
        QString dropStr = QString("Dropped: %1").arg(droppedFrames_);
        painter.setPen(QColor(255, 100, 100));
        painter.drawText(width() / 2 - painter.fontMetrics().horizontalAdvance(dropStr) / 2, 20, dropStr);
    }
}

// ============================================================================
// Audio Handling
// ============================================================================

void PreviewPlayer::startAudioPlayback() {
    if (!audioOutput_ || muted_) return;
    
    // TODO: Implementar stream de audio desde timeline
    // Por ahora, solo inicializamos el dispositivo
    /*
    audioDevice_ = audioOutput_->start();
    if (audioDevice_) {
        // Conectar buffer de audio
    }
    */
}

void PreviewPlayer::stopAudioPlayback() {
    if (audioOutput_) {
        audioOutput_->stop();
    }
    audioDevice_.reset();
}

void PreviewPlayer::updateAudioLevels() {
    // TODO: Implementar medición de niveles de audio en tiempo real
    audioLevelLeft_ = 0.0f;
    audioLevelRight_ = 0.0f;
    emit audioLevelChanged(audioLevelLeft_, audioLevelRight_);
}

// ============================================================================
// Frame Timing
// ============================================================================

void PreviewPlayer::calculateFrameTiming() {
    frameInterval_ = static_cast<qint64>(1000.0 / config_.targetFPS);
}

void PreviewPlayer::scheduleNextFrame() {
    if (state_ == PlaybackState::Playing) {
        frameTimer_->start(frameInterval_);
    }
}

qint64 PreviewPlayer::getFrameTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ============================================================================
// Helpers
// ============================================================================

void PreviewPlayer::updateState(PlaybackState newState) {
    state_ = newState;
}

void PreviewPlayer::notifyMarkerIfCrossed(ccos::core::Time oldTime, ccos::core::Time newTime) {
    for (const auto& [markerTime, label] : markers_) {
        if (oldTime < markerTime && newTime >= markerTime) {
            emit markerReached(markerTime, label);
        }
    }
}

QRectF PreviewPlayer::fitRectKeepAspect(const QRectF& source, const QRectF& target) {
    qreal sourceRatio = source.width() / source.height();
    qreal targetRatio = target.width() / target.height();
    
    QRectF result;
    
    if (sourceRatio > targetRatio) {
        result.setHeight(target.height());
        result.setWidth(result.height() * sourceRatio);
        result.moveLeft(target.left());
        result.moveTop(target.top() + (target.height() - result.height()) / 2);
    } else {
        result.setWidth(target.width());
        result.setHeight(result.width() / sourceRatio);
        result.moveTop(target.top());
        result.moveLeft(target.left() + (target.width() - result.width()) / 2);
    }
    
    return result;
}

} // namespace ccos::ui
