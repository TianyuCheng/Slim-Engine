#ifndef SLIM_UTILITY_SCENEGRAPH_H
#define SLIM_UTILITY_SCENEGRAPH_H

#include <list>
#include <deque>
#include <string>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include "core/commands.h"
#include "core/renderframe.h"
#include "utility/interface.h"
#include "utility/transform.h"
#include "utility/camera.h"
#include "utility/mesh.h"
#include "utility/material.h"
#include "utility/boundingbox.h"
#include "utility/rendergraph.h"

namespace slim {

    class Scene : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        friend class SceneIterator;

        // constructor & destructor
        explicit Scene() = default;
        explicit Scene(const std::string &name, Scene* parent = nullptr);
        virtual ~Scene() = default;

        // setters
        void SetMesh(Mesh* mesh);
        void SetMaterial(Material* material);
        void SetDraw(uint32_t firstVertex, uint32_t vertexCount);
        void SetDrawIndexed(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexOffset);

        // getters
        const std::string& GetName() const { return name; }
        const Transform& GetTransform() const { return transform; }
        bool IsVisible() const { return visible; }
        Mesh* GetMesh() const { return mesh; }
        Material* GetMaterial() const { return material; }

        // scene node hierarchy
        void AddChild(Scene* child);
        void MoveTo(Scene* parent);

        // transform
        void Scale(float x, float y, float z);
        void Rotate(const glm::vec3& axis, float radians);
        void Rotate(float x, float y, float z, float w);
        void Translate(float x, float y, float z);
        void SetTransform(const glm::mat4& transform);
        void SetBoundingBox(const BoundingBox& boundingBox);

        // traversal-based operations
        void ForEach(const std::function<bool(Scene*)> &callback);
        void Update();

    protected:
        std::string name = "";
        Transform transform;
        BoundingBox boundingBox;

        // hierarchy
        Scene* parent = nullptr;
        std::list<Scene*> children = {};

        // properties
        SmartPtr<Mesh> mesh = nullptr;
        SmartPtr<Material> material = nullptr;

    public:
        // draw params
        bool visible = true;
        bool indexed = true;
        struct {
            uint32_t first = 0;
            uint32_t count = 0;
            uint32_t offset = 0;
        } draw;
    };

    // -------------------------------------------------------------

    class SceneManager : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:

        template <typename SceneNode, typename...Args>
        SceneNode* Create(Args...args) {
            SceneNode* node = new SceneNode(args...);
            nodes.push_back(node);
            return node;
        }

    private:
        std::vector<Scene*> roots;
        std::vector<SmartPtr<Scene>> nodes;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_SCENEGRAPH_H
