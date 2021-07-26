#ifndef SLIM_UTILITY_SCENEGRAPH_H
#define SLIM_UTILITY_SCENEGRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include "core/commands.h"
#include "core/renderframe.h"
#include "utility/interface.h"
#include "utility/transform.h"
#include "utility/camera.h"
#include "utility/mesh.h"
#include "utility/material.h"
#include "utility/rendergraph.h"

namespace slim {

    class SceneNode;
    class SceneGraph;
    class SceneComponent;

    class SceneComponent : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        virtual void OnInit(SceneNode *node);
        virtual void OnUpdate(SceneNode *node);
    };

    class SceneNode final : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        friend class Camera;
        friend class SceneGraph;
        friend class SceneComponent;

        explicit        SceneNode();
        explicit        SceneNode(const std::string &name, SceneNode *parent = nullptr);
        virtual         ~SceneNode();

        void            AddChild(SceneNode* child);
        void            AddComponent(SceneComponent* component);

        void            Init();
        void            Update();

        void            SetMesh(Submesh submesh)        { this->submesh = submesh;        }
        void            SetMaterial(Material *material) { this->material = material;      }
        const Submesh&  GetMesh()     const             { return submesh;                 }
        Material*       GetMaterial() const             { return material.get();          }

        auto            begin()       { return children.begin(); }
        auto            begin() const { return children.begin(); }
        auto            end()         { return children.end();   }
        auto            end()   const { return children.end();   }

        const std::string& GetName() const { return name; }

        // mark if this object is visible
        void SetVisible(bool value) { visible = value; }
        bool IsVisible() const { return visible; }

        Transform& GetTransform() { return transform; }
        const Transform& GetTransform() const { return transform; }
        void SetTransform(const Transform& xform) { transform = xform; }

        void Scale(float x, float y, float z);
        void Rotate(const glm::vec3& axis, float radians);
        void Rotate(float x, float y, float z, float w);    // quaternion-based
        void Translate(float x, float y, float z);

        template <typename T>
        T* GetComponent() {
            for (SceneComponent* component : components)
                if (auto obj = dynamic_cast<T*>(component))
                    return obj;
            return nullptr;
        }

    private:
        void Init(SceneNode* parent);
        void Update(SceneNode* parent);
        void UpdateTransformHierarchy(SceneNode* parent);

    private:
        std::string name;                   // node name
        Transform transform;
        SceneNode* parent = nullptr;

        Submesh submesh;
        SmartPtr<Material> material;
        std::vector<SceneNode*> children;
        std::vector<SmartPtr<SceneComponent>> components;

        // states
        bool visible = true;
    }; // end of scene node class


    struct Drawable {
        Submesh  *submesh;
        Pipeline *pipeline;
        float     distance;
        glm::mat4 transform;
    };

    class SceneGraph : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        explicit SceneGraph() = default;
        explicit SceneGraph(SceneNode* node);
        explicit SceneGraph(const std::vector<SceneNode*> &nodes);
        virtual ~SceneGraph() = default;

        void AddRootNode(SceneNode* node);
        void SetRootNode(SceneNode* node);
        void SetRootNodes(const std::vector<SceneNode*> &nodes);

        void Init();
        void Update();
        void Cull(const Camera &camera);
        void Render(const RenderGraph& renderGraph, const Camera &camera, RenderQueue queue, SortingOrder sorting);

        auto begin()       { return roots.begin(); }
        auto begin() const { return roots.begin(); }
        auto end()         { return roots.end();   }
        auto end()   const { return roots.end();   }

    private:
        void Cull(SceneNode *node, const Camera &camera);
        void AddDrawable(RenderQueue renderQueue, Material *material, const Drawable &drawable);

    private:
        std::vector<SceneNode*> roots = {};

        // Culling Results
        using Key = RenderQueue;
        using Val = std::vector<Drawable>;
        using ValList = std::unordered_map<Material*, Val>;
        std::unordered_map<Key, ValList> cullingResults;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_SCENEGRAPH_H
