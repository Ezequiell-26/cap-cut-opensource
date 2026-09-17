#include "RenderGraphTypes.hpp"

namespace ccos::render {

// Implementación de SourceNode
void SourceNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    if (name == "inPoint") {
        // inPoint se maneja como RationalTime, no como variant directo
        // Esta es una simplificación para la interfaz
    } else if (name == "outPoint") {
        // outPoint se maneja como RationalTime
    }
}

std::optional<std::variant<int, float, double, std::string>> SourceNode::getParameter(const std::string& name) const {
    if (name == "filePath") {
        return filePath_;
    }
    return std::nullopt;
}

bool SourceNode::validate() const {
    return !filePath_.empty();
}

// Implementación de TransformNode
void TransformNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    if (name == "posX") {
        posX_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
    } else if (name == "posY") {
        posY_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
    } else if (name == "scaleX") {
        scaleX_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
    } else if (name == "scaleY") {
        scaleY_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
    } else if (name == "rotation") {
        rotation_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
    } else if (name == "anchorX") {
        anchorX_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
    } else if (name == "anchorY") {
        anchorY_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
    }
}

std::optional<std::variant<int, float, double, std::string>> TransformNode::getParameter(const std::string& name) const {
    if (name == "posX") return posX_;
    if (name == "posY") return posY_;
    if (name == "scaleX") return scaleX_;
    if (name == "scaleY") return scaleY_;
    if (name == "rotation") return rotation_;
    if (name == "anchorX") return anchorX_;
    if (name == "anchorY") return anchorY_;
    return std::nullopt;
}

bool TransformNode::validate() const {
    return scaleX_ > 0.0f && scaleY_ > 0.0f;
}

// Implementación de EffectNode
void EffectNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    params_[name] = value;
}

std::optional<std::variant<int, float, double, std::string>> EffectNode::getParameter(const std::string& name) const {
    auto it = params_.find(name);
    if (it != params_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool EffectNode::validate() const {
    return !effectType_.empty();
}

// Implementación de TransitionNode
void TransitionNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    // Los parámetros específicos de transición se pueden agregar aquí
}

std::optional<std::variant<int, float, double, std::string>> TransitionNode::getParameter(const std::string& name) const {
    if (name == "transitionType") {
        return transitionType_;
    }
    return std::nullopt;
}

bool TransitionNode::validate() const {
    return !transitionType_.empty() && duration_.denominator > 0;
}

// Implementación de CompositeNode
void CompositeNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    if (name == "blendMode") {
        int mode = std::visit([](auto&& arg) { return static_cast<int>(arg); }, value);
        blendMode_ = static_cast<BlendMode>(std::clamp(mode, 0, 6));
    } else if (name == "opacity") {
        opacity_ = std::visit([](auto&& arg) { return static_cast<float>(arg); }, value);
        opacity_ = std::clamp(opacity_, 0.0f, 1.0f);
    }
}

std::optional<std::variant<int, float, double, std::string>> CompositeNode::getParameter(const std::string& name) const {
    if (name == "blendMode") return static_cast<int>(blendMode_);
    if (name == "opacity") return opacity_;
    return std::nullopt;
}

bool CompositeNode::validate() const {
    return opacity_ >= 0.0f && opacity_ <= 1.0f;
}

} // namespace ccos::render
