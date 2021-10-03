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

        explicit Node(Node* parent = nullptr);
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

        // create material
        template <typename...Args>
        Material* CreateMaterial(Args...args) {
            Material* material = new Material(device, args...);
            material->SetID(materials.size());
            materials.push_back(material);
            return material;
        }

        // Experimental: bounding box is only used in ray tracing for procedural generation
        void AddAABB(const BoundingBox& aaBox);

        void EnableRayTracing();

        void Build();
        void Clear();

        Device* GetDevice() const { return device; }
        accel::Builder* GetAccelBuilder() const { return accelBuilder; }

        template <typename T>
        void ForEachNode(std::vector<T>& data, std::function<void(T&, Node*)> callback) {
            data.resize(nodes.size());
            uint32_t index = 0;
            for (auto& node : nodes) {
                callback(data[index++], node);
            }
        }

        template <typename T>
        void ForEachMaterial(std::vector<T>& data, std::function<void(T&, Material*)> callback) {
            data.resize(materials.size());
            for (auto& material : materials) {
                callback(data[material->GetID()], material);
            }
        }

        template <typename T>
        void ForEachInstance(std::vector<T>& data, std::function<void(T&, Node*, Mesh*, Material*, uint32_t)> callback) {
            uint32_t instanceId = 0;
            for (auto& node : nodes) {
                for (auto& [mesh, material] : *node) {
                    if (data.size() <= instanceId) {
                        data.resize(instanceId + 1);
                    }
                    callback(data[instanceId], node, mesh, material, instanceId);
                    instanceId++;
                }
            }
        }

        void ForEachInstance(std::function<void(Node*, Mesh*, Material*, uint32_t)> callback) {
            uint32_t instanceId = 0;
            for (auto& node : nodes) {
                for (auto& [mesh, material] : *node) {
                    callback(node, mesh, material, instanceId);
                    instanceId++;
                }
            }
        }

        std::vector<SmartPtr<Node>> nodes;
        std::vector<SmartPtr<Mesh>> meshes;
        std::vector<SmartPtr<Material>> materials;

    private:
        VkBufferUsageFlags GetCommonBufferUsages() const;
        VmaMemoryUsage GetCommonMemoryUsages() const;

        void BuildVertexBuffer(CommandBuffer* commandBuffer, Mesh* mesh,
                               VkBufferUsageFlags bufferUsage,
                               VmaMemoryUsage memoryUsage);

        void BuildIndexBuffer(CommandBuffer* commandBuffer, Mesh* mesh,
                              VkBufferUsageFlags bufferUsage,
                              VmaMemoryUsage memoryUsage);

        void BuildAabbsBuffer(CommandBuffer* commandBuffer,
                              VkBufferUsageFlags bufferUsage,
                              VmaMemoryUsage memoryUsage);

    private:
        SmartPtr<Device> device;
        SmartPtr<accel::Builder> accelBuilder;

        // Experimental: adding bounding box support for procedural generation
        SmartPtr<Node> aabbsNode;
        SmartPtr<Buffer> aabbsBuffer;
        std::vector<BoundingBox> aabbs;
    };

} // end of slim namespace

#endif // SLIM_UTILITY_SCENEGRAPH_H
