#ifndef SLIM_UTILITY_SCENEGRAPH_H
#define SLIM_UTILITY_SCENEGRAPH_H

#include <string>
#include <vector>
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

    class SceneComponent : public ReferenceCountable {
    public:
        virtual void OnInit(SceneNode *node);
        virtual void OnUpdate(SceneNode *node);
    };

    class SceneNode final : public ReferenceCountable {
    public:
        friend class Camera;
        friend class SceneGraph;
        friend class SceneComponent;

        explicit   SceneNode();
        explicit   SceneNode(const std::string &name);
        virtual    ~SceneNode();

        SceneNode* AddChild(const std::string &name);
        void       AddComponent(SceneComponent* component);

        void       ResetTransform();
        void       Translate(float tx, float ty, float tz);
        void       Scale(float sx, float sy, float sz);
        void       Rotate(const glm::vec3 &axis, float radians);

        void       Init();
        void       Update();
        void       Render(const RenderGraph &graph, const Camera &camera);

        void           SetMesh(Submesh submesh)        { this->submesh = submesh;        }
        void           SetMaterial(Material *material) { this->material = material;      }
        const Submesh& GetMesh()     const             { return submesh;                 }
        Material*      GetMaterial() const             { return material.get();          }

        auto       begin()       { return children.begin(); }
        auto       begin() const { return children.begin(); }
        auto       end()         { return children.end();   }
        auto       end()   const { return children.end();   }

        template <typename T>
        T* GetComponent() {
            for (SceneComponent* component : components)
                if (auto obj = dynamic_cast<T*>(component))
                    return obj;
            return nullptr;
        }

        // mark if this object is visible
        void SetVisible(bool value) { visible = value; }
        bool IsVisible() const { return visible; }

        // mark if this object is culled or not
        void SetCulled(bool value) { culled = value; }
        bool IsCulled() const { return culled; }

        // set the distsance from camera to object
        void SetDistanceToCamera(float distance) { distanceToCamera = distance; }
        float GetDistanceToCamera() const { return distanceToCamera; }

        const glm::mat4& GetTransform() const { return xform; }
        const std::string& GetName() const { return name; }

    private:
        void DecomposeTransform();

    private:
        std::string name;                   // node name
        glm::mat4   xform = glm::mat4(1.0); // node xform (local)
        Transform transform;

        Submesh submesh;
        SmartPtr<Material> material;
        std::vector<SmartPtr<SceneNode>> children;
        std::vector<SmartPtr<SceneComponent>> components;

        // states
        bool changed = false;
        bool culled = false;
        bool visible = true;
        float distanceToCamera = 0.0f;
    }; // end of scene node class

    using Scene = SceneNode;

} // end of namespace slim

#endif // end of SLIM_UTILITY_SCENEGRAPH_H
