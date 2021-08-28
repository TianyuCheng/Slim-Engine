#ifndef SLIM_UTILITY_SCENEGRAPH_H
#define SLIM_UTILITY_SCENEGRAPH_H

#include <list>
#include <vector>
#include "core/commands.h"
#include "utility/mesh.h"
#include "utility/material.h"
#include "utility/interface.h"
#include "utility/transform.h"
#include "utility/boundingbox.h"

namespace slim {

    class Scene {
    public:

        // node
        // manages one or multiple instances with hierarchy
        class Node : public NotCopyable, public NotMovable, public ReferenceCountable {
            friend class Builder;

        public:

            explicit Node();
            explicit Node(const std::string& name, Node* parent = nullptr);

            void AddChild(Node* child);
            void MoveTo(Node* parent);

            // mesh
            void SetDraw(Mesh* mesh, Material* material);
            void AddDraw(Mesh* mesh, Material* material);

            // getters
            const std::string& GetName() const    { return name; }
            const Transform& GetTransform() const { return transform; }
            bool IsVisible() const                { return visible; }

            // transform
            void Scale(float x, float y, float z);
            void Rotate(const glm::vec3& axis, float radians);
            void Rotate(float x, float y, float z, float w);
            void Translate(float x, float y, float z);
            void SetTransform(const Transform& transform);
            void ApplyTransform();
            VkTransformMatrixKHR GetVkTransformMatrix() const;

            // traversal-based operations
            void ForEach(const std::function<bool(Scene::Node*)> &callback);

            // geometry traversal
            auto begin()       { return drawables.begin(); }
            auto end()         { return drawables.end();   }
            auto begin() const { return drawables.begin(); }
            auto end()   const { return drawables.end();   }

            void* pNext = nullptr; // user pointer

        private:
            std::string name = "anonymous";
            Node* parent = nullptr;
            Transform transform = glm::mat4(1.0);
            std::list<Node*> children = {};
            std::vector<std::tuple<Mesh*, Material*>> drawables = {};
            bool visible = true;
        };


        // builder:
        // source of data
        class Builder : public NotCopyable, public NotMovable, public ReferenceCountable {
        public:

            explicit Builder(Device* device);

            // create scene node
            template <typename...Args>
            Node* CreateNode(Args...args) {
                Node* node = new Node(args...);
                nodes.push_back(node);
                return node;
            }

            // create geometry
            Mesh* CreateMesh() {
                Mesh* mesh = new Mesh();
                meshes.push_back(mesh);
                return mesh;
            }

            void Build();
            void Clear();

        private:
            std::tuple<uint64_t, uint64_t> CalculateBufferSizes() const;

        private:
            SmartPtr<Device> device;
            std::vector<SmartPtr<Node>> nodes;
            std::vector<SmartPtr<Mesh>> meshes;
            SmartPtr<Buffer> vertexBuffer;
            SmartPtr<Buffer> indexBuffer;
            VkBufferUsageFlags commonBufferUsageFlags = (VkBufferUsageFlags) VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        };

    };

} // end of slim namespace

#endif // SLIM_UTILITY_SCENEGRAPH_H
