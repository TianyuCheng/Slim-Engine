#ifndef SLIM_UTILITY_GLTF_H
#define SLIM_UTILITY_GLTF_H

#include <glm/glm.hpp>
#include "core/commands.h"
#include "utility/mesh.h"
#include "utility/scenegraph.h"
#include "utility/boundingbox.h"
#include "utility/interface.h"

namespace slim::gltf {

    enum class AlphaMode : uint32_t {
        Opaque, Mask, Blend
    };

    struct alignas(128) MaterialData {
        glm::vec4 baseColor                    = glm::vec4(1.0, 1.0, 1.0, 1.0);
        glm::vec4 emissive                     = glm::vec4(0.0, 0.0, 0.0, 1.0);

        // metalness, roughness, occlusion, normal scale
        float     metalness                    = 1.0f;
        float     roughness                    = 1.0f;
        float     occlusion                    = 1.0f;
        float     normalScale                  = 1.0f;

        // texture coord set
        int       baseColorTexCoordSet         = -1;
        int       emissiveTexCoordSet          = -1;
        int       occlusionTexCoordSet         = -1;
        int       normalTexCoordSet            = -1;
        int       metallicRoughnessTexCoordSet = -1;

        // texture info
        int       baseColorTexture             = -1;
        int       emissiveTexture              = -1;
        int       occlusionTexture             = -1;
        int       normalTexture                = -1;
        int       metallicRoughnessTexture     = -1;

        // sampler info
        int       baseColorSampler             = -1;
        int       emissiveSampler              = -1;
        int       occlusionSampler             = -1;
        int       normalSampler                = -1;
        int       metallicRoughnessSampler     = -1;

        // other information
        AlphaMode alphaMode                    =  AlphaMode::Opaque;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::vec4 color0;
        glm::vec4 joints0;
        glm::vec4 weights0;
    };

    struct Primitive {
        VkPrimitiveTopology topology;
        BoundingBox         boundingBox;
        SmartPtr<scene::Mesh>      mesh;
        SmartPtr<scene::Material>  material;
    };

    struct MeshData {
        std::vector<Primitive> primitives;
    };

    struct Scene {
        std::string           name;
        SmartPtr<scene::Node> root;
    };

    struct Model : public ReferenceCountable {
        std::vector<Scene>                     scenes;
        std::vector<MeshData>                  meshes;
        std::vector<SmartPtr<scene::Node>>     nodes;
        std::vector<SmartPtr<scene::Material>> materials;
        std::vector<SmartPtr<Sampler>>         samplers;
        std::vector<SmartPtr<GPUImage>>        images;

        void Load(scene::Builder* builder, const std::string& path, bool verbose = false);

        scene::Node* GetScene(int index) const;
        scene::Node* GetScene(const std::string& name) const;
    };


} // end of slim::gltf

#endif // SLIM_UTILITY_GLTF_H
