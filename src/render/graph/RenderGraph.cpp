#include "RenderGraph.hpp"

#include <algorithm>
#include <stack>
#include <stdexcept>

namespace ccos::render {

void SourceNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    (void)value;
    if (name == "inPoint" || name == "outPoint") {
        throw RenderGraphException("Use setInPoint/setOutPoint for time values");
    }
}

std::optional<std::variant<int, float, double, std::string>>
SourceNode::getParameter(const std::string& name) const {
    if (name == "filePath") return filePath_;
    return std::nullopt;
}

bool SourceNode::validate() const {
    return !filePath_.empty();
}

void TransformNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    if (const auto* val = std::get_if<float>(&value)) {
        if (name == "posX") posX_ = *val;
        else if (name == "posY") posY_ = *val;
        else if (name == "scaleX") scaleX_ = *val;
        else if (name == "scaleY") scaleY_ = *val;
        else if (name == "rotation") rotation_ = *val;
        else if (name == "anchorX") anchorX_ = *val;
        else if (name == "anchorY") anchorY_ = *val;
    }
}

std::optional<std::variant<int, float, double, std::string>>
TransformNode::getParameter(const std::string& name) const {
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

void EffectNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    params_[name] = value;
}

std::optional<std::variant<int, float, double, std::string>>
EffectNode::getParameter(const std::string& name) const {
    const auto it = params_.find(name);
    return it == params_.end() ? std::nullopt : std::optional<std::variant<int, float, double, std::string>>(it->second);
}

bool EffectNode::validate() const {
    return !effectType_.empty();
}

void TransitionNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    (void)name;
    (void)value;
}

std::optional<std::variant<int, float, double, std::string>>
TransitionNode::getParameter(const std::string& name) const {
    (void)name;
    return std::nullopt;
}

bool TransitionNode::validate() const {
    return !transitionType_.empty() && duration_.numerator >= 0;
}

void CompositeNode::setParameter(const std::string& name, const std::variant<int, float, double, std::string>& value) {
    if (const auto* val = std::get_if<float>(&value)) {
        if (name == "opacity") opacity_ = std::clamp(*val, 0.0f, 1.0f);
    } else if (const auto* val = std::get_if<int>(&value)) {
        if (name == "blendMode") blendMode_ = static_cast<BlendMode>(*val);
    }
}

std::optional<std::variant<int, float, double, std::string>>
CompositeNode::getParameter(const std::string& name) const {
    if (name == "opacity") return opacity_;
    if (name == "blendMode") return static_cast<int>(blendMode_);
    return std::nullopt;
}

bool CompositeNode::validate() const {
    return opacity_ >= 0.0f && opacity_ <= 1.0f;
}

void RenderGraph::removeNode(const NodeId& id) {
    const auto it = nodes_.find(id);
    if (it == nodes_.end()) throw NodeNotFoundException(id);

    for (auto& [source, targets] : adjacencyList_) {
        (void)source;
        targets.erase(std::remove(targets.begin(), targets.end(), id), targets.end());
    }
    adjacencyList_.erase(id);
    nodes_.erase(it);
}

std::shared_ptr<RenderNode> RenderGraph::getNode(const NodeId& id) {
    const auto it = nodes_.find(id);
    if (it == nodes_.end()) throw NodeNotFoundException(id);
    return it->second;
}

std::shared_ptr<const RenderNode> RenderGraph::getNode(const NodeId& id) const {
    const auto it = nodes_.find(id);
    if (it == nodes_.end()) throw NodeNotFoundException(id);
    return it->second;
}

void RenderGraph::connect(const NodeId& from, const NodeId& to) {
    if (!hasNode(from)) throw NodeNotFoundException(from);
    if (!hasNode(to)) throw NodeNotFoundException(to);
    if (from == to) throw InvalidConnectionException("self-connections are not allowed");

    auto& targets = adjacencyList_[from];
    if (std::find(targets.begin(), targets.end(), to) != targets.end()) return;

    targets.push_back(to);
    getNode(to)->addInput(from);

    if (hasCycle()) {
        targets.pop_back();
        getNode(to)->removeInput(from);
        throw CycleDetectedException();
    }
}

void RenderGraph::disconnect(const NodeId& from, const NodeId& to) {
    const auto it = adjacencyList_.find(from);
    if (it == adjacencyList_.end()) return;

    auto& targets = it->second;
    targets.erase(std::remove(targets.begin(), targets.end(), to), targets.end());
    if (hasNode(to)) getNode(to)->removeInput(from);
}

GraphValidationResult RenderGraph::validate() const {
    GraphValidationResult result{true, {}, {}};

    for (const auto& [id, node] : nodes_) {
        if (!node || !node->validate()) {
            result.isValid = false;
            result.errors.push_back("Node '" + id + "' failed validation");
        }
    }

    if (hasCycle()) {
        result.isValid = false;
        result.errors.push_back("Render graph contains cycles - this is not allowed");
    }

    if (nodes_.empty()) {
        result.warnings.push_back("Render graph is empty");
    } else if (getRootNodes().empty()) {
        result.warnings.push_back("No root nodes found - graph may be invalid");
    }

    return result;
}

std::vector<NodeId> RenderGraph::getExecutionOrder() const {
    if (hasCycle()) throw CycleDetectedException();

    std::map<NodeId, int> inDegree;
    for (const auto& [id, _] : nodes_) inDegree[id] = 0;

    for (const auto& [source, targets] : adjacencyList_) {
        (void)source;
        for (const auto& target : targets) {
            if (inDegree.find(target) == inDegree.end()) throw NodeNotFoundException(target);
            ++inDegree[target];
        }
    }

    std::queue<NodeId> queue;
    for (const auto& [id, degree] : inDegree) if (degree == 0) queue.push(id);

    std::vector<NodeId> order;
    order.reserve(nodes_.size());
    while (!queue.empty()) {
        const NodeId current = queue.front();
        queue.pop();
        order.push_back(current);

        const auto it = adjacencyList_.find(current);
        if (it == adjacencyList_.end()) continue;
        for (const auto& neighbor : it->second) {
            if (--inDegree[neighbor] == 0) queue.push(neighbor);
        }
    }

    if (order.size() != nodes_.size()) throw CycleDetectedException();
    return order;
}

std::vector<NodeId> RenderGraph::getRootNodes() const {
    std::set<NodeId> hasInputs;
    for (const auto& [source, targets] : adjacencyList_) {
        (void)source;
        for (const auto& target : targets) hasInputs.insert(target);
    }

    std::vector<NodeId> roots;
    for (const auto& [id, _] : nodes_) {
        if (hasInputs.find(id) == hasInputs.end()) roots.push_back(id);
    }
    return roots;
}

std::vector<NodeId> RenderGraph::getLeafNodes() const {
    std::vector<NodeId> leaves;
    for (const auto& [id, _] : nodes_) {
        const auto it = adjacencyList_.find(id);
        if (it == adjacencyList_.end() || it->second.empty()) leaves.push_back(id);
    }
    return leaves;
}

bool RenderGraph::hasCycle() const {
    std::set<NodeId> visited;
    std::set<NodeId> recStack;

    for (const auto& [id, _] : nodes_) {
        if (visited.find(id) == visited.end() && hasCycleDFS(id, visited, recStack)) return true;
    }
    return false;
}

bool RenderGraph::hasCycleDFS(const NodeId& node, std::set<NodeId>& visited,
                              std::set<NodeId>& recStack) const {
    visited.insert(node);
    recStack.insert(node);

    const auto it = adjacencyList_.find(node);
    if (it != adjacencyList_.end()) {
        for (const auto& neighbor : it->second) {
            if (visited.find(neighbor) == visited.end()) {
                if (hasCycleDFS(neighbor, visited, recStack)) return true;
            } else if (recStack.find(neighbor) != recStack.end()) {
                return true;
            }
        }
    }

    recStack.erase(node);
    return false;
}

std::map<NodeId, std::vector<NodeId>> RenderGraph::buildReverseGraph() const {
    std::map<NodeId, std::vector<NodeId>> reverse;
    for (const auto& [source, targets] : adjacencyList_) {
        for (const auto& target : targets) reverse[target].push_back(source);
    }
    return reverse;
}

RenderGraphBuilder& RenderGraphBuilder::addSource(const NodeId& id, const std::string& filePath) {
    graph_->createNode<SourceNode>(id, filePath);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addTransform(const NodeId& id) {
    graph_->createNode<TransformNode>(id);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addEffect(const NodeId& id, const std::string& effectType) {
    graph_->createNode<EffectNode>(id, effectType);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addTransition(const NodeId& id, const std::string& type, RationalTime duration) {
    graph_->createNode<TransitionNode>(id, type, duration);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addComposite(const NodeId& id) {
    graph_->createNode<CompositeNode>(id);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::connect(const NodeId& from, const NodeId& to) {
    graph_->connect(from, to);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setResolution(uint32_t width, uint32_t height) {
    graph_->setOutputResolution({width, height});
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setFrameRate(double fps) {
    graph_->setFrameRate(fps);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setDurationSeconds(double secs) {
    graph_->setDuration(RationalTime::fromSeconds(secs));
    return *this;
}

std::shared_ptr<RenderGraph> RenderGraphBuilder::build() {
    const auto result = graph_->validate();
    if (!result.isValid) {
        std::string errorMsg = "Invalid render graph: ";
        for (const auto& error : result.errors) errorMsg += error + "; ";
        throw RenderGraphException(errorMsg);
    }
    return graph_;
}

} // namespace ccos::render
