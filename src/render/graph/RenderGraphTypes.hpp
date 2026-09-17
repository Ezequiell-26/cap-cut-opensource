#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ccos::render {

using NodeId = std::string;
using TrackId = std::string;
using ClipId = std::string;

enum class NodeType {
    Source,
    Transform,
    Effect,
    Transition,
    Composite,
    AudioMixer,
    Encoder
};

struct Resolution {
    uint32_t width;
    uint32_t height;

    bool operator==(const Resolution& other) const {
        return width == other.width && height == other.height;
    }

    static Resolution HD() { return {1920, 1080}; }
    static Resolution UHD() { return {3840, 2160}; }
    static Resolution SD() { return {720, 480}; }
};

enum class ColorSpace {
    Rec709,
    Rec2020,
    P3,
    Linear
};

enum class BitDepth {
    Bit8,
    Bit10,
    Bit12,
    Float32
};

struct RationalTime {
    int64_t numerator = 0;
    int64_t denominator = 1;

    RationalTime() = default;

    RationalTime(int64_t n, int64_t d)
        : numerator(n)
        , denominator(d) {
        if (denominator <= 0) throw std::invalid_argument("RationalTime denominator must be positive");
    }

    double seconds() const {
        return static_cast<double>(numerator) / static_cast<double>(denominator);
    }

    static RationalTime fromSeconds(double secs) {
        if (!std::isfinite(secs)) throw std::invalid_argument("RationalTime requires a finite number");
        constexpr int64_t denom = 1'000'000;
        const double scaled = secs * static_cast<double>(denom);
        if (scaled > static_cast<double>(std::numeric_limits<int64_t>::max()) ||
            scaled < static_cast<double>(std::numeric_limits<int64_t>::min())) {
            throw std::out_of_range("RationalTime is outside the supported range");
        }
        return {static_cast<int64_t>(scaled), denom};
    }

    bool operator==(const RationalTime& other) const {
        return seconds() == other.seconds();
    }
};

class RenderNode {
public:
    virtual ~RenderNode() = default;

    [[nodiscard]] NodeId id() const { return id_; }
    [[nodiscard]] NodeType type() const { return type_; }

    void addInput(const NodeId& input) {
        if (std::find(inputs_.begin(), inputs_.end(), input) == inputs_.end()) inputs_.push_back(input);
    }

    void removeInput(const NodeId& input) {
        inputs_.erase(std::remove(inputs_.begin(), inputs_.end(), input), inputs_.end());
    }

    [[nodiscard]] const std::vector<NodeId>& inputs() const { return inputs_; }

    virtual void setParameter(const std::string& name,
                              const std::variant<int, float, double, std::string>& value) = 0;
    virtual std::optional<std::variant<int, float, double, std::string>>
    getParameter(const std::string& name) const = 0;
    virtual bool validate() const = 0;

protected:
    RenderNode(NodeId id, NodeType type)
        : id_(std::move(id))
        , type_(type) {}

    NodeId id_;
    NodeType type_;
    std::vector<NodeId> inputs_;
};

class SourceNode : public RenderNode {
public:
    SourceNode(NodeId id, const std::string& filePath)
        : RenderNode(std::move(id), NodeType::Source)
        , filePath_(filePath) {}

    const std::string& filePath() const { return filePath_; }
    void setInPoint(RationalTime time) { inPoint_ = time; }
    void setOutPoint(RationalTime time) { outPoint_ = time; }
    RationalTime inPoint() const { return inPoint_; }
    RationalTime outPoint() const { return outPoint_; }

    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    std::string filePath_;
    RationalTime inPoint_{0, 1};
    RationalTime outPoint_{0, 1};
};

class TransformNode : public RenderNode {
public:
    explicit TransformNode(NodeId id)
        : RenderNode(std::move(id), NodeType::Transform) {}

    void setPosition(float x, float y) { posX_ = x; posY_ = y; }
    void setScale(float sx, float sy) { scaleX_ = sx; scaleY_ = sy; }
    void setRotation(float degrees) { rotation_ = degrees; }
    void setAnchorPoint(float x, float y) { anchorX_ = x; anchorY_ = y; }

    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    float posX_ = 0.0f;
    float posY_ = 0.0f;
    float scaleX_ = 1.0f;
    float scaleY_ = 1.0f;
    float rotation_ = 0.0f;
    float anchorX_ = 0.5f;
    float anchorY_ = 0.5f;
};

class EffectNode : public RenderNode {
public:
    EffectNode(NodeId id, const std::string& effectType)
        : RenderNode(std::move(id), NodeType::Effect)
        , effectType_(effectType) {}

    const std::string& effectType() const { return effectType_; }
    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    std::string effectType_;
    std::map<std::string, std::variant<int, float, double, std::string>> params_;
};

class TransitionNode : public RenderNode {
public:
    TransitionNode(NodeId id, const std::string& transitionType, RationalTime duration)
        : RenderNode(std::move(id), NodeType::Transition)
        , transitionType_(transitionType)
        , duration_(duration) {}

    const std::string& transitionType() const { return transitionType_; }
    RationalTime duration() const { return duration_; }

    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    std::string transitionType_;
    RationalTime duration_;
};

class CompositeNode : public RenderNode {
public:
    enum class BlendMode {
        Normal,
        Add,
        Multiply,
        Screen,
        Overlay,
        SoftLight,
        HardLight
    };

    explicit CompositeNode(NodeId id)
        : RenderNode(std::move(id), NodeType::Composite) {}

    void setBlendMode(BlendMode mode) { blendMode_ = mode; }
    BlendMode blendMode() const { return blendMode_; }
    void setOpacity(float opacity) { opacity_ = opacity; }
    float opacity() const { return opacity_; }

    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    BlendMode blendMode_ = BlendMode::Normal;
    float opacity_ = 1.0f;
};

struct GraphValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

} // namespace ccos::render
