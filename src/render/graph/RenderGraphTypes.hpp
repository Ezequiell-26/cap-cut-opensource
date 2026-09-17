#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <chrono>
#include <cstdint>

namespace ccos::render {

// Identificadores únicos estables
using NodeId = std::string;
using TrackId = std::string;
using ClipId = std::string;

// Tipos de nodos en el grafo de render
enum class NodeType {
    Source,      // Fuente de media (video/audio)
    Transform,   // Transformación geométrica
    Effect,      // Efecto visual
    Transition,  // Transición entre clips
    Composite,   // Composición de múltiples capas
    AudioMixer,  // Mezcla de audio
    Encoder      // Codificador final
};

// Configuración de resolución
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

// Formato de color
enum class ColorSpace {
    Rec709,
    Rec2020,
    P3,
    Linear
};

// Profundidad de color
enum class BitDepth {
    Bit8,
    Bit10,
    Bit12,
    Float32
};

// Frame racional para timing preciso
struct RationalTime {
    int64_t numerator;
    int64_t denominator;
    
    double seconds() const {
        return static_cast<double>(numerator) / denominator;
    }
    
    bool operator<(const RationalTime& other) const {
        return numerator * other.denominator < other.numerator * denominator;
    }
    
    bool operator==(const RationalTime& other) const {
        return numerator * other.denominator == other.numerator * denominator;
    }
    
    RationalTime operator+(const RationalTime& other) const {
        return {
            numerator * other.denominator + other.numerator * denominator,
            denominator * other.denominator
        };
    }
    
    static RationalTime fromSeconds(double secs) {
        const int64_t denom = 1000000;
        return {static_cast<int64_t>(secs * denom), denom};
    }
};

// Nodo base del grafo de render
class RenderNode {
public:
    virtual ~RenderNode() = default;
    
    NodeId id() const { return id_; }
    NodeType type() const { return type_; }
    
    // Inputs y outputs
    void addInput(const NodeId& input) { inputs_.push_back(input); }
    const std::vector<NodeId>& inputs() const { return inputs_; }
    
    // Configuración específica por tipo
    virtual void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) = 0;
    virtual std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const = 0;
    
    // Validación
    virtual bool validate() const = 0;
    
protected:
    RenderNode(NodeId id, NodeType type) : id_(std::move(id)), type_(type) {}
    
    NodeId id_;
    NodeType type_;
    std::vector<NodeId> inputs_;
};

// Nodo fuente (media file)
class SourceNode : public RenderNode {
public:
    SourceNode(NodeId id, const std::string& filePath)
        : RenderNode(std::move(id), NodeType::Source), filePath_(filePath) {}
    
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

// Nodo de transformación (escala, rotación, posición)
class TransformNode : public RenderNode {
public:
    TransformNode(NodeId id) : RenderNode(std::move(id), NodeType::Transform) {}
    
    void setPosition(float x, float y) { posX_ = x; posY_ = y; }
    void setScale(float sx, float sy) { scaleX_ = sx; scaleY_ = sy; }
    void setRotation(float degrees) { rotation_ = degrees; }
    void setAnchorPoint(float x, float y) { anchorX_ = x; anchorY_ = y; }
    
    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    float posX_ = 0, posY_ = 0;
    float scaleX_ = 1.0f, scaleY_ = 1.0f;
    float rotation_ = 0.0f;
    float anchorX_ = 0.5f, anchorY_ = 0.5f;
};

// Nodo de efecto visual
class EffectNode : public RenderNode {
public:
    EffectNode(NodeId id, const std::string& effectType)
        : RenderNode(std::move(id), NodeType::Effect), effectType_(effectType) {}
    
    const std::string& effectType() const { return effectType_; }
    
    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    std::string effectType_;
    std::map<std::string, std::variant<int, float, double, std::string>> params_;
};

// Nodo de transición
class TransitionNode : public RenderNode {
public:
    TransitionNode(NodeId id, const std::string& transitionType, RationalTime duration)
        : RenderNode(std::move(id), NodeType::Transition), 
          transitionType_(transitionType), duration_(duration) {}
    
    const std::string& transitionType() const { return transitionType_; }
    RationalTime duration() const { return duration_; }
    
    void setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) override;
    std::optional<std::variant<int, float, double, std::string>> getParameter(const std::string& name) const override;
    bool validate() const override;

private:
    std::string transitionType_;
    RationalTime duration_;
};

// Nodo de composición (mezcla de capas)
class CompositeNode : public RenderNode {
public:
    enum class BlendMode {
        Normal, Add, Multiply, Screen, Overlay, SoftLight, HardLight
    };
    
    CompositeNode(NodeId id) : RenderNode(std::move(id), NodeType::Composite) {}
    
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

// Resultado de validación del grafo
struct GraphValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

} // namespace ccos::render
