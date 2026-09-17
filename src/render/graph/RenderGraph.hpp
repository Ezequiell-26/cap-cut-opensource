#pragma once

#include "RenderGraphTypes.hpp"
#include <map>
#include <set>
#include <queue>
#include <functional>
#include <stdexcept>
#include <algorithm>

namespace ccos::render {

// Excepciones específicas del grafo de render
class RenderGraphException : public std::runtime_error {
public:
    explicit RenderGraphException(const std::string& msg) : std::runtime_error(msg) {}
};

class CycleDetectedException : public RenderGraphException {
public:
    CycleDetectedException() : RenderGraphException("Cycle detected in render graph") {}
};

class NodeNotFoundException : public RenderGraphException {
public:
    explicit NodeNotFoundException(const NodeId& id) 
        : RenderGraphException("Node not found: " + id) {}
};

class InvalidConnectionException : public RenderGraphException {
public:
    explicit InvalidConnectionException(const std::string& msg)
        : RenderGraphException("Invalid connection: " + msg) {}
};

// Grafo de render principal
class RenderGraph {
public:
    RenderGraph() = default;
    
    // Gestión de nodos
    template<typename T, typename... Args>
    std::shared_ptr<T> createNode(Args&&... args) {
        auto node = std::make_shared<T>(std::forward<Args>(args)...);
        nodes_[node->id()] = node;
        return node;
    }
    
    void removeNode(const NodeId& id);
    
    std::shared_ptr<RenderNode> getNode(const NodeId& id);
    std::shared_ptr<const RenderNode> getNode(const NodeId& id) const;
    
    bool hasNode(const NodeId& id) const {
        return nodes_.find(id) != nodes_.end();
    }
    
    // Conexiones entre nodos
    void connect(const NodeId& from, const NodeId& to);
    void disconnect(const NodeId& from, const NodeId& to);
    
    // Validación del grafo
    GraphValidationResult validate() const;
    
    // Orden topológico para ejecución
    std::vector<NodeId> getExecutionOrder() const;
    
    // Nodos raíz (sin inputs) y hoja (sin outputs)
    std::vector<NodeId> getRootNodes() const;
    std::vector<NodeId> getLeafNodes() const;
    
    // Configuración global
    void setOutputResolution(Resolution res) { outputResolution_ = res; }
    Resolution outputResolution() const { return outputResolution_; }
    
    void setColorSpace(ColorSpace cs) { colorSpace_ = cs; }
    ColorSpace colorSpace() const { return colorSpace_; }
    
    void setFrameRate(double fps) { frameRate_ = fps; }
    double frameRate() const { return frameRate_; }
    
    void setDuration(RationalTime duration) { duration_ = duration; }
    RationalTime duration() const { return duration_; }

private:
    // Detección de ciclos usando DFS
    bool hasCycle() const;
    bool hasCycleDFS(const NodeId& node, 
                     std::set<NodeId>& visited,
                     std::set<NodeId>& recStack) const;
    
    // Grafo inverso para encontrar outputs
    std::map<NodeId, std::vector<NodeId>> buildReverseGraph() const;
    
    std::map<NodeId, std::shared_ptr<RenderNode>> nodes_;
    std::map<NodeId, std::vector<NodeId>> adjacencyList_;
    
    Resolution outputResolution_{1920, 1080};
    ColorSpace colorSpace_ = ColorSpace::Rec709;
    double frameRate_ = 30.0;
    RationalTime duration_{0, 1};
};

// Builder pattern para construcción fluida de grafos
class RenderGraphBuilder {
public:
    explicit RenderGraphBuilder(std::shared_ptr<RenderGraph> graph) 
        : graph_(std::move(graph)) {}
    
    RenderGraphBuilder& addSource(const NodeId& id, const std::string& filePath);
    RenderGraphBuilder& addTransform(const NodeId& id);
    RenderGraphBuilder& addEffect(const NodeId& id, const std::string& effectType);
    RenderGraphBuilder& addTransition(const NodeId& id, const std::string& type, RationalTime duration);
    RenderGraphBuilder& addComposite(const NodeId& id);
    
    RenderGraphBuilder& connect(const NodeId& from, const NodeId& to);
    
    RenderGraphBuilder& setResolution(uint32_t width, uint32_t height);
    RenderGraphBuilder& setFrameRate(double fps);
    RenderGraphBuilder& setDurationSeconds(double secs);
    
    std::shared_ptr<RenderGraph> build();
    
private:
    std::shared_ptr<RenderGraph> graph_;
};

} // namespace ccos::render
