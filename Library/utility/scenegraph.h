#ifndef SLIM_UTILITY_SCENEGRAPH_H
#define SLIM_UTILITY_SCENEGRAPH_H

#include <list>
#include <deque>
#include <string>
#include <variant>
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

    using DrawCommand = VkDrawIndirectCommand;
    using DrawIndexed = VkDrawIndexedIndirectCommand;
    using DrawVariant = std::variant<DrawCommand, DrawIndexed>;

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
        void SetDraw(const DrawCommand& command);
        void SetDraw(const DrawIndexed& command);

        // getters
        const std::string& GetName() const { return name; }
        const Transform& GetTransform() const { return transform; }
        Mesh* GetMesh() const { return mesh; }
        Material* GetMaterial() const { return material; }
        DrawVariant& GetDraw() { return drawCommand; }
        const DrawVariant& GetDraw() const { return drawCommand; }
        bool IsVisible() const { return visible; }

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

        // draw params
        bool visible = true;
        DrawVariant drawCommand;
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

        void Clear();

    private:
        std::vector<Scene*> roots;
        std::vector<SmartPtr<Scene>> nodes;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_SCENEGRAPH_H
