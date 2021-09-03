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
#include "utility/rtbuilder.h"

namespace slim::scene {

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
        bool HasDraw() const { return !drawables.empty(); }
        size_t NumDraws() const { return drawables.size(); }

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
        void ForEach(const std::function<bool(scene::Node*)> &callback);

        // geometry traversal
        auto begin()       { return drawables.begin(); }
        auto end()         { return drawables.end();   }
        auto begin() const { return drawables.begin(); }
        auto end()   const { return drawables.end();   }

        void* pNext = nullptr; // user pointer

    private:
        std::string name = "anonymous";

        // hierarchy
        Node* parent = nullptr;
        std::list<Node*> children = {};

        // transform data
        Transform transform = glm::mat4(1.0);

        // drawables
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

        void EnableRayTracing();
        void EnableASCompaction();

        void Build();
        void Clear();

        accel::Builder* GetAccelBuilder() const { return accelBuilder; }

        std::vector<SmartPtr<Node>> nodes;
        std::vector<SmartPtr<Mesh>> meshes;

    private:
        VkBufferUsageFlags GetCommonBufferUsages() const;
        VmaMemoryUsage GetCommonMemoryUsages() const;

        void BuildVertexBuffer(CommandBuffer* commandBuffer, Mesh* mesh,
                               VkBufferUsageFlags bufferUsage,
                               VmaMemoryUsage memoryUsage);

        void BuildIndexBuffer(CommandBuffer* commandBuffer, Mesh* mesh,
                              VkBufferUsageFlags bufferUsage,
                              VmaMemoryUsage memoryUsage);

        void BuildTransformBuffer(CommandBuffer*,
                                  VkBufferUsageFlags bufferUsage,
                                  VmaMemoryUsage memoryUsage);

    private:
        SmartPtr<Device> device;
        SmartPtr<Buffer> vertexBuffer;
        SmartPtr<Buffer> indexBuffer;
        SmartPtr<Buffer> transformBuffer;
        SmartPtr<accel::Builder> accelBuilder;
    };

} // end of slim namespace

#endif // SLIM_UTILITY_SCENEGRAPH_H
