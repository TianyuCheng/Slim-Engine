#include <deque>
#include <unordered_map>
#include "utility/scenegraph.h"

using namespace slim;

// type shortcuts
using Key           = Material*;
using ValAscending  = std::vector<SceneNode*>;
using ValDescending = std::vector<SceneNode*>;
using ValUnordered  = std::vector<SceneNode*>;
using MapAscending  = std::unordered_map<Key, ValAscending>;
using MapDescending = std::unordered_map<Key, ValDescending>;
using MapUnordered  = std::unordered_map<Key, ValUnordered>;

// -------------------------------------------------------------

bool CompareNodeAscending(SceneNode* left, SceneNode* right) {
    return left->GetDistanceToCamera() < right->GetDistanceToCamera();
}

bool CompareNodeDescending(SceneNode* left, SceneNode* right) {
    return left->GetDistanceToCamera() > right->GetDistanceToCamera();
}

template <typename MapType>
void AddToMapList(MapType& container, Material* material, SceneNode *node) {
    auto it = container.find(material);
    if (it == container.end()) {
        container.insert(std::make_pair(material, ValAscending()));
        it = container.find(material);
    }
    it->second.push_back(node);
}

template <typename MapType, typename Comparator>
void SortMapList(MapType& container, const Comparator &comparator) {
    for (auto &kv : container) {
        std::sort(kv.second.begin(), kv.second.end(), comparator);
    }
}

// -------------------------------------------------------------

void SceneComponent::OnInit(SceneNode *) {
}

void SceneComponent::OnUpdate(SceneNode *) {
}

SceneNode::SceneNode() : name("") {
}

SceneNode::SceneNode(const std::string &name) : name(name) {
}

SceneNode* SceneNode::AddChild(const std::string &name) {
    SceneNode *node = new SceneNode(name);
    children.push_back(node);
    return node;
}

SceneNode::~SceneNode() {
    children.clear();
    components.clear();
}

void SceneNode::Init() {
    DecomposeTransform();

    // initialize all components
    for (auto &component : components)
        component->OnInit(this);

    // initialize all children
    for (auto &child : children)
        child->Init();
}

void SceneNode::Update() {
    DecomposeTransform();

    // update all components
    for (auto &component : components)
        component->OnUpdate(this);

    // update all children
    for (auto &child : children)
        child->Update();
}

void SceneNode::AddComponent(SceneComponent* component) {
    components.push_back(component);
}

void SceneNode::ResetTransform() {
    xform = glm::mat4(1.0f);
}

void SceneNode::DecomposeTransform() {
    if (changed) {
        transform = Transform(xform);
        changed = false;
    }
}

void SceneNode::Translate(float tx, float ty, float tz) {
    xform = glm::translate(xform, glm::vec3(tx, ty, tz));
    changed = true;
}

void SceneNode::Scale(float sx, float sy, float sz) {
    xform = glm::scale(xform, glm::vec3(sx, sy, sz));
    changed = true;
}

void SceneNode::Rotate(const glm::vec3 &axis, float radians) {
    xform = glm::rotate(xform, radians, axis);
    changed = true;
}

void SceneNode::Render(const RenderGraph &graph, const Camera &camera) {
    // quick test
    if (this->IsCulled() || !this->IsVisible()) return;

    // We should draw opque objects front to back;
    // background is for things like skybox;
    // unordered is for things that does not need an order, e.g. OIT
    // We should draw transparent objects afterwards, but sorted back to front.
    MapAscending opaque;
    MapAscending background;
    MapUnordered unordered;
    MapDescending transparent;

    // node traversal
    std::deque<SceneNode*> nodes;
    nodes.push_back(this);
    while (!nodes.empty()) {
        // get the front node
        SceneNode *node = nodes.front();

        // find a queue to fit this object in
        if (Material *material = node->GetMaterial())  {
            Material::Queue queue = material->GetMaterialQueue();

            switch (queue) {
                case Material::Queue::Opaque:      AddToMapList(opaque,      material, node); break;
                case Material::Queue::Background:  AddToMapList(background,  material, node); break;
                case Material::Queue::Unordered:   AddToMapList(unordered,   material, node); break;
                case Material::Queue::Transparent: AddToMapList(transparent, material, node); break;
                default: throw std::runtime_error("[SceneGraph] unhandled material queue type");
            }
        }

        // add children
        for (auto &child : node->children) {
            if (!child->IsCulled() && this->IsVisible())
                nodes.push_back(child);
        }

        // update queue
        nodes.pop_front();
    }

    // sort objects in the map
    SortMapList(opaque, CompareNodeAscending);
    SortMapList(background, CompareNodeAscending);
    SortMapList(transparent, CompareNodeDescending);

    // structured render information for argument passing
    RenderInfo renderInfo = {};
    renderInfo.renderPass = graph.GetRenderPass();
    renderInfo.renderFrame = graph.GetRenderFrame();
    renderInfo.commandBuffer = graph.GetGraphicsCommandBuffer();
    renderInfo.camera = &camera;

    #define RENDER(LIST)                                        \
    for (auto &kv : LIST) {                                     \
        auto &material = kv.first;                              \
        auto &queue = kv.second;                                \
        renderInfo.material = material;                         \
        renderInfo.sceneNodeData = queue.data();                \
        renderInfo.sceneNodeCount = queue.size();               \
        material->Bind(renderInfo);                             \
        material->PrepareMaterial(renderInfo);                  \
        for (auto node : queue) {                               \
            renderInfo.sceneNode = node;                        \
            material->PrepareSceneNode(renderInfo);             \
            node->submesh.Bind(renderInfo.commandBuffer);       \
            node->submesh.Draw(renderInfo.commandBuffer);       \
        }                                                       \
    }
    RENDER(opaque);
    RENDER(background);
    RENDER(unordered);
    RENDER(transparent);
    #undef RENDER
}
