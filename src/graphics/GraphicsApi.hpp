#pragma once
/**
 * CCOS - Graphics & Animation API
 * Integración con Skia, Lottie, NanoSVG y otras librerías MIT de gráficos
 * Licencias: BSD (Skia), MIT (Lottie, NanoSVG), Apache 2.0
 */

#include <vector>
#include <string>
#include <memory>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkImage.h>

namespace ccos::graphics {

struct VectorPath {
    std::vector<SkPoint> points;
    std::vector<uint8_t> verbs;
    bool closed = false;
};

struct Gradient {
    enum Type { Linear, Radial, Sweep };
    Type type;
    std::vector<SkColor> colors;
    std::vector<float> positions;
    SkPoint startPoint;
    SkPoint endPoint;
};

struct Shadow {
    float blurRadius;
    SkColor color;
    float offsetX;
    float offsetY;
};

class GraphicsEngine {
public:
    static GraphicsEngine& getInstance();
    
    // Inicialización
    bool initializeSkia();
    bool initializeLottie(const std::string& lottiePath);
    bool initializeNanoSVG();
    
    // Crear canvas offscreen
    sk_sp<SkSurface> createSurface(int width, int height, bool transparent = true);
    
    // Drawing primitives
    void drawLine(SkCanvas* canvas, SkPoint start, SkPoint end, 
                  SkColor color, float strokeWidth = 1.0f);
    
    void drawRect(SkCanvas* canvas, SkRect rect, 
                  SkColor fillColor, SkColor strokeColor = SK_ColorTRANSPARENT,
                  float strokeWidth = 0);
    
    void drawRoundedRect(SkCanvas* canvas, SkRect rect, float radius,
                         SkColor fillColor, SkColor strokeColor = SK_ColorTRANSPARENT);
    
    void drawCircle(SkCanvas* canvas, SkPoint center, float radius,
                    SkColor fillColor, SkColor strokeColor = SK_ColorTRANSPARENT);
    
    void drawEllipse(SkCanvas* canvas, SkRect bounds,
                     SkColor fillColor, SkColor strokeColor = SK_ColorTRANSPARENT);
    
    void drawPath(SkCanvas* canvas, const SkPath& path,
                  SkColor fillColor, SkColor strokeColor = SK_ColorTRANSPARENT,
                  float strokeWidth = 1.0f);
    
    // Gradients
    void applyLinearGradient(SkPaint* paint, const Gradient& gradient);
    void applyRadialGradient(SkPaint* paint, const Gradient& gradient);
    
    // Text rendering
    void drawText(SkCanvas* canvas, const std::string& text,
                  SkPoint position, SkFont font, SkColor color);
    
    void drawTextOnPath(SkCanvas* canvas, const std::string& text,
                        const SkPath& path, SkFont font, SkColor color);
    
    struct TextMetrics {
        float width;
        float height;
        float ascent;
        float descent;
    };
    TextMetrics measureText(const std::string& text, const SkFont& font);
    
    // Image operations
    void drawImage(SkCanvas* canvas, sk_sp<SkImage> image,
                   SkPoint position, float opacity = 1.0f);
    
    void drawImageRect(SkCanvas* canvas, sk_sp<SkImage> image,
                       SkRect srcRect, SkRect dstRect,
                       float opacity = 1.0f);
    
    sk_sp<SkImage> loadImage(const std::string& path);
    sk_sp<SkImage> loadSVGGizmo(const std::string& svgData);
    
    // Lottie animation
    class LottieAnimation {
    public:
        bool load(const std::string& jsonPath);
        void render(SkCanvas* canvas, float progress);
        float duration() const;
        int width() const;
        int height() const;
        void setProgress(float progress);
        float getProgress() const;
    private:
        void* animation_ = nullptr; // RLottie handle
        float progress_ = 0.0f;
        int width_ = 0;
        int height_ = 0;
    };
    
    std::shared_ptr<LottieAnimation> loadLottie(const std::string& jsonPath);
    
    // SVG parsing and rendering
    sk_sp<SkPicture> parseSVG(const std::string& svgContent);
    void renderSVG(SkCanvas* canvas, const std::string& svgContent,
                   SkRect bounds);
    
    // Effects
    void applyBlur(SkCanvas* canvas, float sigmaX, float sigmaY);
    void applyDropShadow(SkCanvas* canvas, const Shadow& shadow);
    void applyGlow(SkCanvas* canvas, SkColor glowColor, float intensity);
    
    // Blend modes
    enum BlendMode {
        SrcOver, SrcIn, SrcOut, SrcATop,
        DstOver, DstIn, DstOut, DstATop,
        Xor, Plus, Modulate,
        Screen, Overlay, Darken, Lighten,
        ColorDodge, ColorBurn, HardLight, SoftLight,
        Difference, Exclusion, Multiply, Hue, Saturation, Color, Luminosity
    };
    void setBlendMode(SkCanvas* canvas, BlendMode mode);
    
    // Masking
    void pushMask(SkCanvas* canvas, const SkPath& mask);
    void popMask(SkCanvas* canvas);
    
    // Clipping
    void clipRect(SkCanvas* canvas, SkRect rect);
    void clipPath(SkCanvas* canvas, const SkPath& path);
    
    // Transformations
    void translate(SkCanvas* canvas, float dx, float dy);
    void scale(SkCanvas* canvas, float sx, float sy);
    void rotate(SkCanvas* canvas, float degrees, SkPoint pivot = {0, 0});
    void skew(SkCanvas* canvas, float kx, float ky);
    
    // Save/restore state
    void save(SkCanvas* canvas);
    void restore(SkCanvas* canvas);
    
    // Shape generators
    SkPath createStar(SkPoint center, float outerRadius, float innerRadius,
                      int points);
    
    SkPath createPolygon(SkPoint center, float radius, int sides);
    
    SkPath createArrow(SkPoint start, SkPoint end, float headSize);
    
    SkPath createWave(SkRect bounds, float amplitude, float frequency,
                      float phase = 0);
    
    // Particle systems
    struct Particle {
        SkPoint position;
        SkPoint velocity;
        SkColor color;
        float size;
        float life;
        float maxLife;
    };
    
    class ParticleSystem {
    public:
        void emit(SkPoint position, int count);
        void update(float deltaTime);
        void render(SkCanvas* canvas);
        void setGravity(SkPoint gravity);
        void setEmissionRate(float rate);
    private:
        std::vector<Particle> particles_;
        SkPoint gravity_ = {0, 0};
        float emissionRate_ = 10.0f;
    };
    
    // Morphing entre paths
    SkPath morphPaths(const SkPath& from, const SkPath& to, float t);
    
    // Stroke animations (draw-on effect)
    SkPath animateStroke(const SkPath& path, float progress);
    
    // Generar spritesheets
    sk_sp<SkImage> generateSpriteSheet(
        const std::vector<sk_sp<SkImage>>& frames,
        int columns, int rows);
    
    // Export a diferentes formatos
    bool exportToPNG(sk_sp<SkImage> image, const std::string& path);
    bool exportToJPEG(sk_sp<SkImage> image, const std::string& path, int quality = 90);
    bool exportToWEBP(sk_sp<SkImage> image, const std::string& path, float quality = 0.8f);
    bool exportToSVG(const SkPath& path, const std::string& filename);
    
    // Color utilities
    SkColor hexToColor(const std::string& hex);
    std::string colorToHex(SkColor color);
    SkColor interpolateColor(SkColor from, SkColor to, float t);
    
    // Obtener estado del motor
    bool isInitialized() const { return initialized_; }
    std::string getBackendInfo() const;
    
private:
    GraphicsEngine() = default;
    ~GraphicsEngine() = default;
    GraphicsEngine(const GraphicsEngine&) = delete;
    GraphicsEngine& operator=(const GraphicsEngine&) = delete;
    
    bool initialized_ = false;
    bool skiaInitialized_ = false;
    bool lottieInitialized_ = false;
};

// Herramientas para composición en timeline
class GraphicsTimelineTools {
public:
    // Generar lower thirds animados
    static sk_sp<SkImage> generateLowerThird(const std::string& title,
                                              const std::string& subtitle,
                                              int width = 1920, int height = 1080);
    
    // Generar títulos animados
    static sk_sp<SkImage> generateAnimatedTitle(const std::string& text,
                                                 const std::string& style = "modern");
    
    // Generar transiciones gráficas
    static sk_sp<SkImage> generateTransition(const std::string& type,
                                              int width, int height,
                                              float progress);
    
    // Crear overlays de redes sociales
    static sk_sp<SkImage> generateSocialOverlay(const std::string& platform,
                                                 const std::string& handle);
    
    // Generar watermarks
    static sk_sp<SkImage> generateWatermark(const std::string& text,
                                             SkColor color,
                                             float opacity = 0.3f);
    
    // Crear fondos animados
    static sk_sp<SkImage> generateAnimatedBackground(const std::string& style,
                                                      int width, int height,
                                                      float time);
};

} // namespace ccos::graphics
