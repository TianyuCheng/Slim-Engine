#include "utility/gltf.h"
#include "utility/texture.h"
#include "utility/tinygltf.h"
#include "utility/filesystem.h"

using namespace slim;
using namespace slim::gltf;

void ReadVertexPosition(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // POSITION: VEC3, FLOAT

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC3);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

    uint32_t stride = std::max(bufferView.byteStride, 3 * sizeof(float));
    for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
        const float* p = (float*)(data);
        vertices[i].position = glm::vec3(p[0], p[1], p[2]);
    }
}

void ReadVertexNormal(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // NORMAL: VEC3, FLOAT

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC3);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

    uint32_t stride = std::max(bufferView.byteStride, 3 * sizeof(float));
    for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
        const float* p = (float*)(data);
        vertices[i].normal = glm::vec3(p[0], p[1], p[2]);
    }
}

void ReadVertexTangent(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // TANGENT: VEC3, FLOAT, where w component is a sign value (-1, or 1 indicating the handedness of the tangent basis)

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC4);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

    uint32_t stride = std::max(bufferView.byteStride, 3 * sizeof(float));
    for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
        const float* p = (float*)(data);
        vertices[i].tangent = glm::vec4(p[0], p[1], p[2], p[3]);
    }
}

void ReadVertexTexCoord0(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // TEXCOORD_0: VEC2, FLOAT/UBYTE/USHORT

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC2);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        uint32_t stride = std::max(bufferView.byteStride, 2 * sizeof(float));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const float* p = (float*)(data);
            vertices[i].uv0 = glm::vec2(p[0], p[1]);
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        uint32_t stride = std::max(bufferView.byteStride, 2 * sizeof(unsigned short));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint16_t* p = (uint16_t*)(data);
            vertices[i].uv0 = glm::vec2(p[0] / 65536.0, p[1] / 65536.0);
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        uint32_t stride = std::max(bufferView.byteStride, 2 * sizeof(unsigned char));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint8_t* p = (uint8_t*)(data);
            vertices[i].uv0 = glm::vec2(p[0] / 256.0, p[1] / 256.0);
        }
    }
}

void ReadVertexTexCoord1(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // TEXCOORD_1: VEC2, FLOAT/UBYTE/USHORT

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC2);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        uint32_t stride = std::max(bufferView.byteStride, 2 * sizeof(float));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const float* p = (float*)(data);
            vertices[i].uv1 = glm::vec2(p[0], p[1]);
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        uint32_t stride = std::max(bufferView.byteStride, 2 * sizeof(unsigned short));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint16_t* p = (uint16_t*)(data);
            vertices[i].uv1 = glm::vec2(p[0] / 65536.0, p[1] / 65536.0);
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        uint32_t stride = std::max(bufferView.byteStride, 2 * sizeof(unsigned char));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint8_t* p = (uint8_t*)(data);
            vertices[i].uv1 = glm::vec2(p[0] / 256.0, p[1] / 256.0);
        }
    }
}

void ReadVertexColor0(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // COLOR_0: VEC3/VEC4, FLOAT/UBYTE/USHORT

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC3 ||
           accessor.type == TINYGLTF_TYPE_VEC4);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);

    if (accessor.type == TINYGLTF_TYPE_VEC3) {
        uint32_t stride = std::max(bufferView.byteStride, 3 * sizeof(float));
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
                const float* p = (float*)(data);
                vertices[i].color0 = glm::vec4(p[0], p[1], p[2], 1.0);
            }
        }

        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            uint32_t stride = std::max(bufferView.byteStride, 3 * sizeof(unsigned short));
            for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
                const uint16_t* p = (uint16_t*)(data);
                vertices[i].color0 = glm::vec4(p[0] / 65536.0, p[1] / 65536.0, p[2] / 65536.0, 1.0);
            }
        }

        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            uint32_t stride = std::max(bufferView.byteStride, 3 * sizeof(unsigned char));
            for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
                const uint8_t* p = (uint8_t*)(data);
                vertices[i].color0 = glm::vec4(p[0] / 256.0, p[1] / 256.0, p[2] / 256.0, 1.0);
            }
        }
    }

    else {
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(float));
            for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
                const float* p = (float*)(data);
                vertices[i].color0 = glm::vec4(p[0], p[1], p[2], p[3]);
            }
        }

        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(unsigned short));
            for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
                const uint16_t* p = (uint16_t*)(data);
                vertices[i].color0 = glm::vec4(p[0] / 65536.0, p[1] / 65536.0, p[2] / 65536.0, p[3] / 65536.0);
            }
        }

        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(unsigned char));
            for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
                const uint8_t* p = (uint8_t*)(data);
                vertices[i].color0 = glm::vec4(p[0] / 256.0, p[1] / 256.0, p[2] / 256.0, p[3] / 256.0);
            }
        }
    }
}

void ReadVertexJoints0(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // JOINTS_0: VEC4, FLOAT/USHORT

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC4);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(float));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const float* p = (float*)(data);
            vertices[i].joints0 = glm::vec4(p[0], p[1], p[2], p[3]);
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(unsigned short));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint16_t* p = (uint16_t*)(data);
            vertices[i].joints0 = glm::vec4(p[0] / 65536.0, p[1] / 65536.0, p[2] / 65536.0, p[3] / 65536.0);
        }
    }
}

void ReadVertexWeights0(std::vector<Vertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > vertices.size()) {
        vertices.resize(accessor.count);
    }

    // WEIGHTS_0: VEC4, FLOAT/UBYTE/USHORT

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.type == TINYGLTF_TYPE_VEC4);
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(float));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const float* p = (float*)(data);
            vertices[i].weights0 = glm::vec4(p[0], p[1], p[2], p[3]);
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(unsigned short));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint16_t* p = (uint16_t*)(data);
            vertices[i].weights0 = glm::vec4(p[0] / 65536.0, p[1] / 65536.0, p[2] / 65536.0, p[3] / 65536.0);
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        uint32_t stride = std::max(bufferView.byteStride, 4 * sizeof(unsigned char));
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint8_t* p = (uint8_t*)(data);
            vertices[i].weights0 = glm::vec4(p[0] / 256.0, p[1] / 256.0, p[2] / 256.0, p[3] / 256.0);
        }
    }
}

void ReadIndices(std::vector<uint32_t>& indices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    if (accessor.count > indices.size()) {
        indices.resize(accessor.count);
    }

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    char* data = (char*) model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;

    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
           accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        uint32_t stride = bufferView.byteStride ? bufferView.byteOffset : sizeof(uint16_t);
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint16_t* p = (uint16_t*)(data);
            indices[i] = p[0];
        }
    }

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        uint32_t stride = bufferView.byteStride ? bufferView.byteOffset : sizeof(uint32_t);
        for (uint32_t i = 0; i < accessor.count; i++, data += stride) {
            const uint32_t* p = (uint32_t*)(data);
            indices[i] = p[0];
        }
    }
}

// https://stackoverflow.com/Questions/5255806/how-to-calculate-tangent-and-binormal
// NOTE: This algorithm does not get me entirely correct normal, I need to investigate what's wrong.
void MakeTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, bool verbose) {
    uint32_t nIndices = indices.size();
    uint32_t inconsistentUvs = 0;
    for (uint32_t i : indices) {
        vertices[i].tangent = glm::vec4(0.0);
    }
    for (uint32_t l = 0; l < nIndices; l++) {
        uint32_t i = indices[l];
        uint32_t j = indices[(l + 1) % nIndices];
        uint32_t k = indices[(l + 2) % nIndices];
        glm::vec3 n = vertices[i].normal;
        glm::vec3 v1 = vertices[j].position - vertices[i].position, v2 = vertices[k].position - vertices[i].position;
        glm::vec2 t1 = vertices[j].uv0 - vertices[i].uv0, t2 = vertices[k].uv0 - vertices[i].uv0;

        // Is the texture flipped?
        float uv2xArea = t1.x * t2.y - t1.y * t2.x;
        if (std::abs(uv2xArea) < 0x1p-20) {
            continue;  // Smaller than 1/2 pixel at 1024x1024
        }

        float flip = uv2xArea > 0 ? 1 : -1;
        // 'flip' or '-flip'; depends on the handedness of the space.
        if (vertices[i].tangent.w != 0 && vertices[i].tangent.w != -flip) {
            ++inconsistentUvs;
        }
        vertices[i].tangent.w = -flip;

        // Project triangle onto tangent plane
        v1 -= n * dot(v1, n);
        v2 -= n * dot(v2, n);
        // Tangent is object space direction of texture coordinates
        glm::vec3 s = glm::normalize((t2.y * v1 - t1.y * v2) * flip);

        // Use angle between projected v1 and v2 as weight
        float angle = std::acos(dot(v1, v2) / (length(v1) * length(v2)));
        vertices[i].tangent += glm::vec4(s * angle, 0);
    }
    for (uint32_t i : indices) {
        glm::vec4& t = vertices[i].tangent;
        vertices[i].tangent = glm::vec4(normalize(glm::vec3(t.x, t.y, t.z)), t.w);
    }
    if (verbose) {
        std::cerr << inconsistentUvs << " inconsistent UVs\n";
    }
}

void LoadSamplers(Device* device, Model &result, const tinygltf::Model &model) {
    for (const auto& sampler : model.samplers) {
        slim::SamplerDesc desc;

        // min filter + mip filter
        if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) desc.MinFilter(VK_FILTER_NEAREST);
        if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR)  desc.MinFilter(VK_FILTER_LINEAR);
        if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
            desc.MinFilter(VK_FILTER_NEAREST);
            desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST);
        }
        if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
            desc.MinFilter(VK_FILTER_NEAREST);
            desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
        }
        if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
            desc.MinFilter(VK_FILTER_LINEAR);
            desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST);
        }
        if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
            desc.MinFilter(VK_FILTER_LINEAR);
            desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
        }

        // max filter
        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) desc.MagFilter(VK_FILTER_NEAREST);
        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR)  desc.MagFilter(VK_FILTER_LINEAR);

        // wrap S
        VkSamplerAddressMode addrS;
        if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)   addrS = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) addrS = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_REPEAT)          addrS = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

        // wrap T
        VkSamplerAddressMode addrT;
        if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)   addrT = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) addrT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_REPEAT)          addrT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

        result.samplers.push_back(new Sampler(device, desc));
    }

    // default sampler
    if (result.samplers.size() == 0) {
        auto desc = SamplerDesc { };
        result.samplers.push_back(new Sampler(device, desc));
    }
}

void LoadImages(Device* device, Model &result, const tinygltf::Model& model, const std::string& basedir) {
    device->Execute([&](CommandBuffer* commandBuffer) {
        for (const auto& image : model.images) {
            std::string filepath = basedir + "/" + image.uri;
            result.images.push_back(TextureLoader::Load2D(commandBuffer, filepath));
        }
    });
}

void LoadMaterials(Model &result, const tinygltf::Model &model, scene::Builder* builder) {
    for (const auto& material : model.materials) {

        // create new material
        auto* mat = builder->CreateMaterial();

        MaterialData factors = {};

        if (material.alphaMode == "OPAQUE") {
            factors.alphaMode = AlphaMode::Opaque;
        }
        else if (material.alphaMode == "MASK") {
            factors.alphaMode = AlphaMode::Mask;
        }
        else if (material.alphaMode == "BLEND") {
            factors.alphaMode = AlphaMode::Blend;
        }

        // pbr metallic roughness
        const auto pbrMetallicRoughness = material.pbrMetallicRoughness;
        factors.baseColor.x = pbrMetallicRoughness.baseColorFactor[0];
        factors.baseColor.y = pbrMetallicRoughness.baseColorFactor[1];
        factors.baseColor.z = pbrMetallicRoughness.baseColorFactor[2];
        factors.baseColor.w = pbrMetallicRoughness.baseColorFactor[3];

        // base color texture
        const auto& baseColorTexture = pbrMetallicRoughness.baseColorTexture;
        if (baseColorTexture.index >= 0) {
            int32_t texture = baseColorTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.baseColorTexCoordSet = baseColorTexture.texCoord;
            factors.baseColorTexture = image;
            factors.baseColorSampler = sampler;
        }

        // metallic roughness
        factors.metalness = pbrMetallicRoughness.metallicFactor;
        factors.roughness = pbrMetallicRoughness.roughnessFactor;
        const auto& roughnessTexture = pbrMetallicRoughness.metallicRoughnessTexture;
        if (roughnessTexture.index >= 0) {
            int32_t texture = roughnessTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.metallicRoughnessTexCoordSet = roughnessTexture.texCoord;
            factors.metallicRoughnessTexture = image;
            factors.metallicRoughnessSampler = sampler;
        }

        // normal texture
        const auto& normalTexture = material.normalTexture;
        factors.normalScale = normalTexture.scale;
        if (normalTexture.index >= 0) {
            int32_t texture = normalTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.normalTexCoordSet = normalTexture.texCoord;
            factors.normalTexture = image;
            factors.normalSampler = sampler;
        }

        // occlusion texture
        const auto& occlusionTexture = material.occlusionTexture;
        factors.occlusion = occlusionTexture.strength;
        if (occlusionTexture.index >= 0) {
            int32_t texture = occlusionTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.occlusionTexCoordSet = occlusionTexture.texCoord;
            factors.occlusionTexture = image;
            factors.occlusionSampler = sampler;
        }

        // emissive texture
        const auto& emissiveTexture = material.emissiveTexture;
        factors.emissive.x = material.emissiveFactor[0];
        factors.emissive.y = material.emissiveFactor[1];
        factors.emissive.z = material.emissiveFactor[2];
        if (emissiveTexture.index >= 0) {
            int32_t texture = emissiveTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.emissiveTexCoordSet = emissiveTexture.texCoord;
            factors.emissiveTexture = image;
            factors.emissiveSampler = sampler;
        }

        mat->SetData(factors);
        result.materials.push_back(mat);
    }
}

void LoadMeshes(Model &result, const tinygltf::Model& model, scene::Builder* builder, bool verbose) {
    for (const auto& mesh : model.meshes) {
        result.meshes.push_back(MeshData { });
        MeshData& gltfmesh = result.meshes.back();
        for (const auto& primitive : mesh.primitives) {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            bool hasTangent = false;

            for (const auto& kv : primitive.attributes) {
                const auto& attrib = kv.first;
                const auto& accessor = model.accessors[kv.second];

                if (attrib == "POSITION") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: POSITION" << std::endl;
                    ReadVertexPosition(vertices, model, accessor);
                }

                else if (attrib == "NORMAL") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: NORMAL" << std::endl;
                    ReadVertexNormal(vertices, model, accessor);
                }

                else if (attrib == "TANGENT") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: TANGENT" << std::endl;
                    ReadVertexTangent(vertices, model, accessor);
                    hasTangent = true;
                }

                else if (attrib == "TEXCOORD_0") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: TEXCOORD_0" << std::endl;
                    ReadVertexTexCoord0(vertices, model, accessor);
                }

                else if (attrib == "TEXCOORD_1") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: TEXCOORD_1" << std::endl;
                    ReadVertexTexCoord1(vertices, model, accessor);
                }

                else if (attrib == "COLOR_0") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: COLOR_0" << std::endl;
                    ReadVertexColor0(vertices, model, accessor);
                }

                else if (attrib == "JOINTS_0") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: JOINTS_0" << std::endl;
                    ReadVertexJoints0(vertices, model, accessor);
                }

                else if (attrib == "WEIGHTS_0") {
                    if (verbose) std::cout << "[LoadModel] Loading vertex attrib: WEIGHTS_0" << std::endl;
                    ReadVertexWeights0(vertices, model, accessor);
                }

            } // end of attribute loop

            if (primitive.indices >= 0) {
                if (verbose) std::cout << "[LoadModel] Loading index attrib" << std::endl;
                ReadIndices(indices, model, model.accessors[primitive.indices]);
            }

            // tangent?
            if (!hasTangent) {
                MakeTangents(vertices, indices, verbose);
            }

            Primitive prim;

            prim.mesh = builder->CreateMesh();
            prim.mesh->SetVertexBuffer(vertices);

            // index
            if (!indices.empty()) {
                prim.mesh->SetIndexBuffer(indices);
            }

            // topology
            assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
            prim.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            // material
            if (primitive.material >= 0) {
                prim.material = result.materials[primitive.material];
            } else {
                assert(!!!"I have not implemented default material!");
            }

            gltfmesh.primitives.push_back(prim);
        } // end of primitive loop
    } // end of mesh loop
}

void LoadNodes(Model &result, const tinygltf::Model &model, scene::Builder* builder) {
    for (const auto &node : model.nodes) {

        // create new node
        auto snode = builder->CreateNode(node.name);
        result.nodes.push_back(snode);

        // initialize scene node transform
        if (node.matrix.size()) {
            glm::mat4 matrix;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    matrix[i][j] = node.matrix[i * 4 + j];
                }
            }
            snode->SetTransform(matrix);
        }

        if (!node.rotation.empty()) {
            snode->Rotate(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
        }

        if (!node.scale.empty()) {
            snode->Translate(node.scale[0], node.scale[1], node.scale[2]);
        }

        if (!node.translation.empty()) {
            snode->Translate(node.translation[0], node.translation[1], node.translation[2]);
        }

        if (node.mesh >= 0) {
            const auto& mesh = result.meshes[node.mesh];
            for (const auto& primitive : mesh.primitives) {
                snode->AddDraw(primitive.mesh, primitive.material);
                snode->AddDraw(primitive.mesh, primitive.material);
            }
        }

    }

    // initialize scene node hierarchy
    for (uint32_t i = 0; i < model.nodes.size(); i++) {
        scene::Node* snode = result.nodes[i];
        for (int child : model.nodes[i].children) {
            snode->AddChild(result.nodes[child]);
        }
    }

}

void LoadScenes(Model& result, const tinygltf::Model& model, scene::Builder* builder) {
    for (const auto &scene : model.scenes) {
        result.scenes.push_back(Scene { });
        Scene& scn = result.scenes.back();
        scn.name = scene.name;
        scn.root = builder->CreateNode(scene.name);
        for (int node : scene.nodes) {
            scn.root->AddChild(result.nodes[node]);
        }
    }
}

void Model::Load(scene::Builder* builder, const std::string& path, bool verbose) {
    // clearing existing data
    scenes.clear();
    meshes.clear();
    nodes.clear();
    materials.clear();
    samplers.clear();
    images.clear();

    Device* device = builder->GetDevice();

    // --------------------------------------------------------------

    std::string name = filesystem::path(path).filename().u8string();
    std::string base = filesystem::path(path).parent_path().u8string();

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    std::string err;
    std::string warn;
    if (verbose) std::cout << "[LoadModel] Loading model: " << name << std::endl;
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path, tinygltf::REQUIRE_ALL);

    if (!warn.empty()) {
        std::cout << "Warn: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cout << "Err: " << err << std::endl;
    }

    if (!ret) {
        std::cout << "Failed to parse glTF" << std::endl;
        throw std::runtime_error("Failed to load model");
    }

    // --------------------------------------------------------------

    Model& result = *this;

    if (verbose) std::cout << "[LoadModel] Loading samplers" << std::endl;
    LoadSamplers(device, result, model);

    if (verbose) std::cout << "[LoadModel] Loading images" << std::endl;
    LoadImages(device, result, model, base);

    if (verbose) std::cout << "[LoadModel] Loading materials" << std::endl;
    LoadMaterials(result, model, builder);

    if (verbose) std::cout << "[LoadModel] Loading meshes" << std::endl;
    LoadMeshes(result, model, builder, verbose);

    if (verbose) std::cout << "[LoadModel] Loading nodes" << std::endl;
    LoadNodes(result, model, builder);

    if (verbose) std::cout << "[LoadModel] Loading scenes" << std::endl;
    LoadScenes(result, model, builder);
}

scene::Node* Model::GetScene(int index) const {
    if (size_t(index) >= scenes.size()) {
        throw std::runtime_error("scene index >= scene.size()");
    }
    return scenes[index].root;
}

scene::Node* Model::GetScene(const std::string& name) const {
    for (const Scene& scene : scenes) {
        if (scene.name == name) {
            return scene.root;
        }
    }
    throw std::runtime_error("Failed to find scene with name == " + name);
    return nullptr;
}
