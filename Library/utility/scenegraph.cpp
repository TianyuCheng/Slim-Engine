#include "utility/scenegraph.h"

using namespace slim;

void SceneComponent::OnInit(SceneNode *) {
}

void SceneComponent::OnUpdate(SceneNode *) {
}

SceneNode::SceneNode() : name("") {
}

SceneNode::SceneNode(const std::string &name, SceneNode *parent) : name(name), parent(parent) {
    if (parent) parent->AddChild(this);
}

void SceneNode::AddChild(SceneNode* child) {
    children.push_back(child);
}

SceneNode::~SceneNode() {
    children.clear();
    components.clear();
}

void SceneNode::Init() {
    Init(nullptr);
}

void SceneNode::Update() {
    Update(nullptr);
}

void SceneNode::Init(SceneNode* parent) {
    UpdateTransformHierarchy(parent);

    // initialize all components
    for (auto &component : components)
        component->OnInit(this);

    // initialize all children
    for (auto &child : children)
        child->Init(this);
}

void SceneNode::Update(SceneNode* parent) {
    UpdateTransformHierarchy(parent);

    // update all components
    for (auto &component : components)
        component->OnUpdate(this);

    // update all children
    for (auto &child : children)
        child->Update(this);
}

void SceneNode::AddComponent(SceneComponent* component) {
    components.push_back(component);
}

void SceneNode::UpdateTransformHierarchy(SceneNode* parent) {
    if (parent) {
        transform.ApplyParentTransform(parent->transform);
    } else {
        transform.ApplyTransform();
    }
}

void SceneNode::Scale(float x, float y, float z) {
    transform.Scale(x, y, z);
}

void SceneNode::Rotate(const glm::vec3& axis, float radians) {
    transform.Rotate(axis, radians);
}

void SceneNode::Rotate(float x, float y, float z, float w) {
    transform.Rotate(x, y, z, w);
}

void SceneNode::Translate(float x, float y, float z) {
    transform.Translate(x, y, z);
}

bool CompareDrawableAscending(const Drawable& left, const Drawable& right) {
    return left.distance < right.distance;
}

bool CompareDrawableDescending(const Drawable& left, const Drawable& right) {
    return left.distance > right.distance;
}

SceneGraph::SceneGraph(SceneNode* node) {
    SetRootNode(node);
}

SceneGraph::SceneGraph(const std::vector<SceneNode*> &nodes) {
    SetRootNodes(nodes);
}

void SceneGraph::AddRootNode(SceneNode *node) {
    roots.push_back(node);
}

void SceneGraph::SetRootNode(SceneNode* node) {
    roots = { node };
}

void SceneGraph::SetRootNodes(const std::vector<SceneNode*> &nodes) {
    roots = nodes;
}

void SceneGraph::AddDrawable(RenderQueue renderQueue, Material *material, const Drawable &drawable) {
    // find the first level render queue
    auto it = cullingResults.find(renderQueue);
    if (it == cullingResults.end()) {
        cullingResults.insert(std::make_pair(renderQueue, ValList()));
        it = cullingResults.find(renderQueue);
    }

    // find the second level material queue
    auto it2 = it->second.find(material);
    if (it2 == it->second.end()) {
        it->second.insert(std::make_pair(material, Val()));
        it2 = it->second.find(material);
    }

    it2->second.push_back(drawable);
}

void SceneGraph::Init() {
    for (SceneNode* node : roots) {
        node->Init();
    }
}

void SceneGraph::Update() {
    for (SceneNode* node : roots) {
        node->Update();
    }
}

void SceneGraph::Cull(const Camera &camera) {
    cullingResults.clear();
    for (SceneNode* node : roots) {
        Cull(node, camera);
    }
}

void SceneGraph::Cull(SceneNode *node, const Camera &camera) {
    // quick test for manually turned off nodes
    if (!node->IsVisible()) return;

    bool drawable = true;

    // do camera culling
    float distance = 0.0f;
    if (drawable && !camera.Cull(node, distance)) {
        drawable = false;
    }

    // check if this node is drawable
    Material* material = node->GetMaterial();
    const auto& mesh = node->GetMesh();
    if (drawable && material && mesh.mesh) {
        for (const auto &pass : *material->GetTechnique()) {
            Drawable drawable = {
                const_cast<Submesh*>(&mesh),
                pass.pipeline,
                distance,
                node->GetTransform().LocalToWorld(),
            };
            AddDrawable(pass.queue, material, drawable);
        }
    }

    // child node traversal
    for (SceneNode* child : *node) {
        Cull(child, camera);
    }
}

void SceneGraph::Render(const RenderGraph& renderGraph, const Camera &camera, RenderQueue queue, SortingOrder sorting) {
    // skip if there is nothing in the render queue
    auto it = cullingResults.find(queue);
    if (it == cullingResults.end()) return;

    RenderPass* renderPass = renderGraph.GetRenderPass();
    RenderFrame* renderFrame = renderGraph.GetRenderFrame();
    CommandBuffer* commandBuffer = renderGraph.GetGraphicsCommandBuffer();

    RenderInfo renderInfo = {};
    renderInfo.camera = const_cast<Camera*>(&camera);
    renderInfo.renderPass = renderPass;
    renderInfo.renderFrame = renderFrame;
    renderInfo.commandBuffer = commandBuffer;

    // draw objects grouped by material
    auto& mapper = it->second;
    for (auto &kv : mapper) {
        Material* material = kv.first;

        // fetch queue index and pipeline layout
        uint32_t queueIndex = material->QueueIndex(queue);
        PipelineLayout* layout = material->Layout(queueIndex);

        // bind material specific pipeline & descriptors
        material->Bind(queueIndex, commandBuffer, renderFrame, renderPass);

        // bind camera for all drawables with this material
        camera.Bind(commandBuffer, renderFrame, layout);

        // sort drawables front to back
        if (sorting == SortingOrder::FrontToback)
            std::sort(kv.second.begin(), kv.second.end(), CompareDrawableAscending);

        // sort drawables back to front
        if (sorting == SortingOrder::BackToFront)
            std::sort(kv.second.begin(), kv.second.end(), CompareDrawableDescending);

        const auto& [offset, _, stages] = layout->GetPushConstant("Xform");

        // draw
        for (const auto& drawable : kv.second) {
            commandBuffer->PushConstants(layout, offset, drawable.transform, stages);
            drawable.submesh->Bind(commandBuffer);
            drawable.submesh->Draw(commandBuffer);
        }
    }

}
