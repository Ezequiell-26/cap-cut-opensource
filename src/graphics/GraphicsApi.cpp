#include "GraphicsApi.hpp"
#include <SkGradientShader.h>
#include <SkBlurImageFilter.h>
#include <SkDropShadowImageFilter.h>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ccos::graphics {

GraphicsEngine& GraphicsEngine::getInstance() {
    static GraphicsEngine instance;
    return instance;
}

bool GraphicsEngine::initializeSkia() {
    skiaInitialized_ = true;
    initialized_ = skiaInitialized_;
    return true;
}

bool GraphicsEngine::initializeLottie(const std::string& lottiePath) {
    (void)lottiePath;
    lottieInitialized_ = true;
    initialized_ = skiaInitialized_ && lottieInitialized_;
    return true;
}

bool GraphicsEngine::initializeNanoSVG() {
    return true;
}

sk_sp<SkSurface> GraphicsEngine::createSurface(int width, int height, bool transparent) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    if (!transparent) {
        info = SkImageInfo::MakeN32Opaque(width, height);
    }
    return SkSurface::MakeRaster(info);
}

void GraphicsEngine::drawLine(SkCanvas* canvas, SkPoint start, SkPoint end, 
                               SkColor color, float strokeWidth) {
    SkPaint paint;
    paint.setColor(color);
    paint.setStrokeWidth(strokeWidth);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setAntiAlias(true);
    canvas->drawLine(start.fX, start.fY, end.fX, end.fY, paint);
}

void GraphicsEngine::drawRect(SkCanvas* canvas, SkRect rect, 
                               SkColor fillColor, SkColor strokeColor,
                               float strokeWidth) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    if (fillColor != SK_ColorTRANSPARENT) {
        paint.setColor(fillColor);
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(rect, paint);
    }
    
    if (strokeColor != SK_ColorTRANSPARENT && strokeWidth > 0) {
        paint.setColor(strokeColor);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(strokeWidth);
        canvas->drawRect(rect, paint);
    }
}

void GraphicsEngine::drawRoundedRect(SkCanvas* canvas, SkRect rect, float radius,
                                      SkColor fillColor, SkColor strokeColor) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    SkRRect rrect;
    rrect.setRectXY(rect, radius, radius);
    
    if (fillColor != SK_ColorTRANSPARENT) {
        paint.setColor(fillColor);
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRRect(rrect, paint);
    }
    
    if (strokeColor != SK_ColorTRANSPARENT) {
        paint.setColor(strokeColor);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawRRect(rrect, paint);
    }
}

void GraphicsEngine::drawCircle(SkCanvas* canvas, SkPoint center, float radius,
                                 SkColor fillColor, SkColor strokeColor) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    if (fillColor != SK_ColorTRANSPARENT) {
        paint.setColor(fillColor);
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(center.fX, center.fY, radius, paint);
    }
    
    if (strokeColor != SK_ColorTRANSPARENT) {
        paint.setColor(strokeColor);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawCircle(center.fX, center.fY, radius, paint);
    }
}

void GraphicsEngine::drawEllipse(SkCanvas* canvas, SkRect bounds,
                                  SkColor fillColor, SkColor strokeColor) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    if (fillColor != SK_ColorTRANSPARENT) {
        paint.setColor(fillColor);
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawOval(bounds, paint);
    }
    
    if (strokeColor != SK_ColorTRANSPARENT) {
        paint.setColor(strokeColor);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawOval(bounds, paint);
    }
}

void GraphicsEngine::drawPath(SkCanvas* canvas, const SkPath& path,
                               SkColor fillColor, SkColor strokeColor,
                               float strokeWidth) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    if (fillColor != SK_ColorTRANSPARENT) {
        paint.setColor(fillColor);
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawPath(path, paint);
    }
    
    if (strokeColor != SK_ColorTRANSPARENT && strokeWidth > 0) {
        paint.setColor(strokeColor);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(strokeWidth);
        canvas->drawPath(path, paint);
    }
}

void GraphicsEngine::applyLinearGradient(SkPaint* paint, const Gradient& gradient) {
    if (gradient.colors.size() < 2) return;
    
    std::vector<SkColor> skColors(gradient.colors.begin(), gradient.colors.end());
    std::vector<SkScalar> positions(gradient.positions.begin(), gradient.positions.end());
    
    auto shader = SkGradientShader::MakeLinear(
        gradient.startPoint, gradient.endPoint,
        skColors.data(), positions.data(), gradient.colors.size(),
        SkTileMode::kClamp);
    
    paint->setShader(shader);
}

void GraphicsEngine::applyRadialGradient(SkPaint* paint, const Gradient& gradient) {
    if (gradient.colors.size() < 2) return;
    
    std::vector<SkColor> skColors(gradient.colors.begin(), gradient.colors.end());
    std::vector<SkScalar> positions(gradient.positions.begin(), gradient.positions.end());
    
    float radius = SkPoint::Distance(gradient.startPoint, gradient.endPoint);
    
    auto shader = SkGradientShader::MakeRadial(
        gradient.startPoint, radius,
        skColors.data(), positions.data(), gradient.colors.size(),
        SkTileMode::kClamp);
    
    paint->setShader(shader);
}

void GraphicsEngine::drawText(SkCanvas* canvas, const std::string& text,
                               SkPoint position, SkFont font, SkColor color) {
    SkPaint paint;
    paint.setColor(color);
    paint.setAntiAlias(true);
    canvas->drawString(text.c_str(), position.fX, position.fY, font, paint);
}

void GraphicsEngine::drawTextOnPath(SkCanvas* canvas, const std::string& text,
                                     const SkPath& path, SkFont font, SkColor color) {
    SkPaint paint;
    paint.setColor(color);
    paint.setAntiAlias(true);
    canvas->drawTextOnPath(text.c_str(), path, nullptr, font, paint);
}

GraphicsEngine::TextMetrics GraphicsEngine::measureText(const std::string& text, const SkFont& font) {
    TextMetrics metrics;
    SkRect bounds;
    font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8, &bounds);
    metrics.width = bounds.width();
    metrics.height = bounds.height();
    metrics.ascent = -bounds.top();
    metrics.descent = bounds.bottom();
    return metrics;
}

void GraphicsEngine::drawImage(SkCanvas* canvas, sk_sp<SkImage> image,
                                SkPoint position, float opacity) {
    if (!image) return;
    
    SkPaint paint;
    paint.setAlpha(static_cast<U8CPU>(opacity * 255));
    canvas->drawImage(image, position.fX, position.fY, SkSamplingOptions(), &paint);
}

void GraphicsEngine::drawImageRect(SkCanvas* canvas, sk_sp<SkImage> image,
                                    SkRect srcRect, SkRect dstRect,
                                    float opacity) {
    if (!image) return;
    
    SkPaint paint;
    paint.setAlpha(static_cast<U8CPU>(opacity * 255));
    canvas->drawImageRect(image, srcRect, dstRect, SkSamplingOptions(), &paint,
                          SkCanvas::kFast_SrcRectConstraint);
}

sk_sp<SkImage> GraphicsEngine::loadImage(const std::string& path) {
    (void)path;
    return nullptr;
}

sk_sp<SkImage> GraphicsEngine::loadSVGGizmo(const std::string& svgData) {
    (void)svgData;
    return nullptr;
}

std::shared_ptr<GraphicsEngine::LottieAnimation> GraphicsEngine::loadLottie(const std::string& jsonPath) {
    auto anim = std::make_shared<LottieAnimation>();
    anim->load(jsonPath);
    return anim;
}

bool GraphicsEngine::LottieAnimation::load(const std::string& jsonPath) {
    (void)jsonPath;
    width_ = 1920;
    height_ = 1080;
    return true;
}

void GraphicsEngine::LottieAnimation::render(SkCanvas* canvas, float progress) {
    (void)canvas;
    (void)progress;
}

float GraphicsEngine::LottieAnimation::duration() const {
    return 1.0f;
}

int GraphicsEngine::LottieAnimation::width() const {
    return width_;
}

int GraphicsEngine::LottieAnimation::height() const {
    return height_;
}

void GraphicsEngine::LottieAnimation::setProgress(float progress) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

float GraphicsEngine::LottieAnimation::getProgress() const {
    return progress_;
}

sk_sp<SkPicture> GraphicsEngine::parseSVG(const std::string& svgContent) {
    (void)svgContent;
    return nullptr;
}

void GraphicsEngine::renderSVG(SkCanvas* canvas, const std::string& svgContent,
                                SkRect bounds) {
    (void)canvas;
    (void)svgContent;
    (void)bounds;
}

void GraphicsEngine::applyBlur(SkCanvas* canvas, float sigmaX, float sigmaY) {
    SkPaint paint;
    paint.setImageFilter(SkBlurImageFilter::Make(sigmaX, sigmaY, nullptr, nullptr));
    canvas->saveLayer(nullptr, &paint);
}

void GraphicsEngine::applyDropShadow(SkCanvas* canvas, const Shadow& shadow) {
    SkPaint paint;
    paint.setImageFilter(SkDropShadowImageFilter::Make(
        shadow.offsetX, shadow.offsetY,
        shadow.blurRadius, shadow.blurRadius,
        shadow.color, nullptr, nullptr));
    canvas->saveLayer(nullptr, &paint);
}

void GraphicsEngine::applyGlow(SkCanvas* canvas, SkColor glowColor, float intensity) {
    Shadow shadow;
    shadow.blurRadius = intensity * 20.0f;
    shadow.color = glowColor;
    shadow.offsetX = 0;
    shadow.offsetY = 0;
    applyDropShadow(canvas, shadow);
}

void GraphicsEngine::setBlendMode(SkCanvas* canvas, BlendMode mode) {
    SkBlendMode skMode = SkBlendMode::kSrcOver;
    switch (mode) {
        case SrcOver: skMode = SkBlendMode::kSrcOver; break;
        case SrcIn: skMode = SkBlendMode::kSrcIn; break;
        case SrcOut: skMode = SkBlendMode::kSrcOut; break;
        case SrcATop: skMode = SkBlendMode::kSrcATop; break;
        case DstOver: skMode = SkBlendMode::kDstOver; break;
        case DstIn: skMode = SkBlendMode::kDstIn; break;
        case DstOut: skMode = SkBlendMode::kDstOut; break;
        case DstATop: skMode = SkBlendMode::kDstATop; break;
        case Xor: skMode = SkBlendMode::kXor; break;
        case Plus: skMode = SkBlendMode::kPlus; break;
        case Modulate: skMode = SkBlendMode::kModulate; break;
        case Screen: skMode = SkBlendMode::kScreen; break;
        case Overlay: skMode = SkBlendMode::kOverlay; break;
        case Darken: skMode = SkBlendMode::kDarken; break;
        case Lighten: skMode = SkBlendMode::kLighten; break;
        case ColorDodge: skMode = SkBlendMode::kColorDodge; break;
        case ColorBurn: skMode = SkBlendMode::kColorBurn; break;
        case HardLight: skMode = SkBlendMode::kHardLight; break;
        case SoftLight: skMode = SkBlendMode::kSoftLight; break;
        case Difference: skMode = SkBlendMode::kDifference; break;
        case Exclusion: skMode = SkBlendMode::kExclusion; break;
        case Multiply: skMode = SkBlendMode::kMultiply; break;
        case Hue: skMode = SkBlendMode::kHue; break;
        case Saturation: skMode = SkBlendMode::kSaturation; break;
        case Color: skMode = SkBlendMode::kColor; break;
        case Luminosity: skMode = SkBlendMode::kLuminosity; break;
    }
    canvas->setBlendMode(skMode);
}

void GraphicsEngine::pushMask(SkCanvas* canvas, const SkPath& mask) {
    canvas->clipPath(mask);
}

void GraphicsEngine::popMask(SkCanvas* canvas) {
    canvas->restore();
}

void GraphicsEngine::clipRect(SkCanvas* canvas, SkRect rect) {
    canvas->clipRect(rect);
}

void GraphicsEngine::clipPath(SkCanvas* canvas, const SkPath& path) {
    canvas->clipPath(path);
}

void GraphicsEngine::translate(SkCanvas* canvas, float dx, float dy) {
    canvas->translate(dx, dy);
}

void GraphicsEngine::scale(SkCanvas* canvas, float sx, float sy) {
    canvas->scale(sx, sy);
}

void GraphicsEngine::rotate(SkCanvas* canvas, float degrees, SkPoint pivot) {
    if (pivot.fX != 0 || pivot.fY != 0) {
        canvas->translate(pivot.fX, pivot.fY);
        canvas->rotate(degrees);
        canvas->translate(-pivot.fX, -pivot.fY);
    } else {
        canvas->rotate(degrees);
    }
}

void GraphicsEngine::skew(SkCanvas* canvas, float kx, float ky) {
    canvas->skew(kx, ky);
}

void GraphicsEngine::save(SkCanvas* canvas) {
    canvas->save();
}

void GraphicsEngine::restore(SkCanvas* canvas) {
    canvas->restore();
}

SkPath GraphicsEngine::createStar(SkPoint center, float outerRadius, float innerRadius,
                                   int points) {
    SkPath path;
    const float PI = 3.14159265f;
    
    for (int i = 0; i < points * 2; ++i) {
        float radius = (i % 2 == 0) ? outerRadius : innerRadius;
        float angle = (i * PI) / points - PI / 2;
        float x = center.fX + radius * std::cos(angle);
        float y = center.fY + radius * std::sin(angle);
        
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    
    path.close();
    return path;
}

SkPath GraphicsEngine::createPolygon(SkPoint center, float radius, int sides) {
    SkPath path;
    const float PI = 3.14159265f;
    
    for (int i = 0; i < sides; ++i) {
        float angle = (2 * PI * i / sides) - PI / 2;
        float x = center.fX + radius * std::cos(angle);
        float y = center.fY + radius * std::sin(angle);
        
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    
    path.close();
    return path;
}

SkPath GraphicsEngine::createArrow(SkPoint start, SkPoint end, float headSize) {
    SkPath path;
    
    float dx = end.fX - start.fX;
    float dy = end.fY - start.fY;
    float length = std::sqrt(dx * dx + dy * dy);
    float ux = dx / length;
    float uy = dy / length;
    
    path.moveTo(start.fX, start.fY);
    path.lineTo(end.fX - headSize, end.fY);
    
    float perpX = -uy;
    float perpY = ux;
    
    path.lineTo(end.fX, end.fY);
    path.lineTo(end.fX - headSize + perpX * headSize * 0.5f, 
                end.fY - headSize + perpY * headSize * 0.5f);
    path.close();
    
    return path;
}

SkPath GraphicsEngine::createWave(SkRect bounds, float amplitude, float frequency,
                                   float phase) {
    SkPath path;
    const float PI = 3.14159265f;
    
    float centerY = (bounds.fTop + bounds.fBottom) / 2;
    float wavelength = bounds.width() / frequency;
    
    for (float x = bounds.fLeft; x <= bounds.fRight; x += 2) {
        float t = (x - bounds.fLeft) / wavelength * 2 * PI + phase;
        float y = centerY + amplitude * std::sin(t);
        
        if (x == bounds.fLeft) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    
    return path;
}

void GraphicsEngine::ParticleSystem::emit(SkPoint position, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = position;
        p.velocity = {static_cast<float>((rand() % 100 - 50) / 100.0f),
                      static_cast<float>((rand() % 100 - 50) / 100.0f)};
        p.color = SK_ColorWHITE;
        p.size = static_cast<float>(rand() % 10 + 5);
        p.life = 1.0f;
        p.maxLife = 1.0f;
        particles_.push_back(p);
    }
}

void GraphicsEngine::ParticleSystem::update(float deltaTime) {
    for (auto it = particles_.begin(); it != particles_.end();) {
        it->position.fX += it->velocity.fX * deltaTime;
        it->position.fY += it->velocity.fY * deltaTime;
        it->velocity.fY += gravity_.fY * deltaTime;
        it->life -= deltaTime / emissionRate_;
        
        if (it->life <= 0) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }
}

void GraphicsEngine::ParticleSystem::render(SkCanvas* canvas) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    for (const auto& p : particles_) {
        paint.setAlpha(static_cast<U8CPU>(p.life * 255));
        paint.setColor(p.color);
        canvas->drawCircle(p.position.fX, p.position.fY, p.size, paint);
    }
}

void GraphicsEngine::ParticleSystem::setGravity(SkPoint gravity) {
    gravity_ = gravity;
}

void GraphicsEngine::ParticleSystem::setEmissionRate(float rate) {
    emissionRate_ = rate;
}

SkPath GraphicsEngine::morphPaths(const SkPath& from, const SkPath& to, float t) {
    (void)from;
    (void)to;
    (void)t;
    return SkPath();
}

SkPath GraphicsEngine::animateStroke(const SkPath& path, float progress) {
    (void)path;
    (void)progress;
    return SkPath();
}

sk_sp<SkImage> GraphicsEngine::generateSpriteSheet(
    const std::vector<sk_sp<SkImage>>& frames,
    int columns, int rows) {
    (void)frames;
    (void)columns;
    (void)rows;
    return nullptr;
}

bool GraphicsEngine::exportToPNG(sk_sp<SkImage> image, const std::string& path) {
    (void)image;
    (void)path;
    return false;
}

bool GraphicsEngine::exportToJPEG(sk_sp<SkImage> image, const std::string& path, int quality) {
    (void)image;
    (void)path;
    (void)quality;
    return false;
}

bool GraphicsEngine::exportToWEBP(sk_sp<SkImage> image, const std::string& path, float quality) {
    (void)image;
    (void)path;
    (void)quality;
    return false;
}

bool GraphicsEngine::exportToSVG(const SkPath& path, const std::string& filename) {
    (void)path;
    (void)filename;
    return false;
}

SkColor GraphicsEngine::hexToColor(const std::string& hex) {
    if (hex.size() < 6) return SK_ColorBLACK;
    
    unsigned int r = 0, g = 0, b = 0;
    std::stringstream ss;
    ss << std::hex << hex.substr(0, 6);
    ss >> r >> g >> b;
    
    return SkColorSetRGB(static_cast<U8CPU>(r), static_cast<U8CPU>(g), static_cast<U8CPU>(b));
}

std::string GraphicsEngine::colorToHex(SkColor color) {
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%02X%02X%02X",
             SkColorGetR(color), SkColorGetG(color), SkColorGetB(color));
    return std::string(buffer);
}

SkColor GraphicsEngine::interpolateColor(SkColor from, SkColor to, float t) {
    U8CPU r = static_cast<U8CPU>(SkColorGetR(from) * (1 - t) + SkColorGetR(to) * t);
    U8CPU g = static_cast<U8CPU>(SkColorGetG(from) * (1 - t) + SkColorGetG(to) * t);
    U8CPU b = static_cast<U8CPU>(SkColorGetB(from) * (1 - t) + SkColorGetB(to) * t);
    return SkColorSetRGB(r, g, b);
}

std::string GraphicsEngine::getBackendInfo() const {
    std::string info = "GraphicsEngine Backends: ";
    if (skiaInitialized_) info += "Skia ";
    if (lottieInitialized_) info += "Lottie ";
    if (!initialized_) info += "(Not initialized)";
    return info;
}

sk_sp<SkImage> GraphicsTimelineTools::generateLowerThird(const std::string& title,
                                                          const std::string& subtitle,
                                                          int width, int height) {
    (void)title;
    (void)subtitle;
    (void)width;
    (void)height;
    return nullptr;
}

sk_sp<SkImage> GraphicsTimelineTools::generateAnimatedTitle(const std::string& text,
                                                             const std::string& style) {
    (void)text;
    (void)style;
    return nullptr;
}

sk_sp<SkImage> GraphicsTimelineTools::generateTransition(const std::string& type,
                                                          int width, int height,
                                                          float progress) {
    (void)type;
    (void)width;
    (void)height;
    (void)progress;
    return nullptr;
}

sk_sp<SkImage> GraphicsTimelineTools::generateSocialOverlay(const std::string& platform,
                                                             const std::string& handle) {
    (void)platform;
    (void)handle;
    return nullptr;
}

sk_sp<SkImage> GraphicsTimelineTools::generateWatermark(const std::string& text,
                                                         SkColor color,
                                                         float opacity) {
    (void)text;
    (void)color;
    (void)opacity;
    return nullptr;
}

sk_sp<SkImage> GraphicsTimelineTools::generateAnimatedBackground(const std::string& style,
                                                                  int width, int height,
                                                                  float time) {
    (void)style;
    (void)width;
    (void)height;
    (void)time;
    return nullptr;
}

} // namespace ccos::graphics
