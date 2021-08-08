#include "utility/scenegraph.h"

using namespace slim;

Scene::Scene(const std::string &name, Scene* parent) : name(name) {
    MoveTo(parent);
}

void Scene::SetDraw(Mesh* mesh, Material* material, const DrawCommand& command) {
    primitives.clear();
    primitives.push_back(Primitive {
        mesh, material, command
    });
}

void Scene::SetDraw(Mesh* mesh, Material* material, const DrawIndexed& command) {
    primitives.clear();
    primitives.push_back(Primitive {
        mesh, material, command
    });
}

void Scene::AddDraw(Mesh* mesh, Material* material, const DrawCommand& command) {
    primitives.push_back(Primitive {
        mesh, material, command
    });
}

void Scene::AddDraw(Mesh* mesh, Material* material, const DrawIndexed& command) {
    primitives.push_back(Primitive {
        mesh, material, command
    });
}

void Scene::AddChild(Scene* child) {
    child->MoveTo(this);
}

void Scene::MoveTo(Scene *parent) {
    // check if parent is the same
    // skip if no movement
    if (this->parent == parent) {
        return;
    }

    // check if this node has a parent
    // remove this node from parent
    if (this->parent) {
        this->parent->children.remove(this);
    }

    // add to parent
    this->parent = parent;
    parent->children.push_back(this);
}

void Scene::Scale(float x, float y, float z) {
    transform.Scale(x, y, z);
}

void Scene::Rotate(const glm::vec3& axis, float radians) {
    transform.Rotate(axis, radians);
}

void Scene::Rotate(float x, float y, float z, float w) {
    transform.Rotate(x, y, z, w);
}

void Scene::Translate(float x, float y, float z) {
    transform.Translate(x, y, z);
}

void Scene::SetTransform(const glm::mat4& transform) {
    this->transform = transform;
}

void Scene::SetTransform(const Transform& transform) {
    this->transform = transform;
}

void Scene::SetBoundingBox(const BoundingBox& boundingBox) {
    this->boundingBox = boundingBox;
}

void Scene::ForEach(const std::function<bool(Scene*)> &callback) {
    std::deque<Scene*> nodes = { this };
    while (!nodes.empty()) {
        Scene* node = nodes.front();

        // check callback to determine if we need to continue traversal
        if (callback(node)) {
            for (Scene* child : node->children) {
                nodes.push_back(child);
            }
        }

        nodes.pop_front();
    }
}

void Scene::Update() {
    ForEach([](Scene* node) {
        if (node->parent) {
            node->transform.ApplyParentTransform(node->parent->transform);
        } else {
            node->transform.ApplyTransform();
        }
        return true;
    });
}


void SceneManager::Clear() {
    roots.clear();
    nodes.clear();
}
