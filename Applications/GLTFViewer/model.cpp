#include "config.h"
#include "model.h"

GraphicsPipelineDesc CreateGLTFPipelineDesc(const std::string &name, Shader* vShader, Shader* fShader) {
    // https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md
    return
        GraphicsPipelineDesc()
            .SetName(name)
            .AddVertexBinding(0, sizeof(GLTFVertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                { 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(GLTFVertex, pos)      },
                { 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(GLTFVertex, normal)   },
                { 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(GLTFVertex, tangent)  },
                { 3, VK_FORMAT_R32G32_SFLOAT,       offsetof(GLTFVertex, uv0)      },
                { 4, VK_FORMAT_R32G32_SFLOAT,       offsetof(GLTFVertex, uv1)      },
                { 5, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(GLTFVertex, color0)   },
                { 6, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(GLTFVertex, joints0)  },
                { 7, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(GLTFVertex, weights0) },
             })
            .SetVertexShader(vShader)
            .SetFragmentShader(fShader)
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetDepthTest(VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetSampleCount(msaa)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Camera",                   0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("Model",                    2, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                .AddBinding("MaterialFactors",          1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("BaseColorTexture",         1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("MetallicRoughnessTexture", 1, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("NormalTexture",            1, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("EmissiveTexture",          1, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("OcclusionTexture",         1, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
                .AddBinding("DFGLUT",                   3, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("EnvironmentTexture",       3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding("IrradianceTexture",        3, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            );

    // Base Color:
    //  When material is metal, the base color is the specific measured reflectance value at normal incidence (F0).
    //  When material is non-metal, the base color represents the reflected diffuse color of the material. A linear 4% is used.
}

GLTFAssetManager::GLTFAssetManager(Device* device) : device(device) {
    manager = SlimPtr<SceneManager>();

    // vertex shader
    auto vShaderPbr = SlimPtr<spirv::VertexShader>(device, "main", "shaders/gltf.vert.spv");

    // fragment shader
    auto fShaderPbr = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/gltf.frag.spv");

    // opaque pbr technique
    techniqueOpaque = SlimPtr<Technique>();
    techniqueOpaque->AddPass(RenderQueue::Opaque, CreateGLTFPipelineDesc("PBR Opaque", vShaderPbr, fShaderPbr));

    // mask pbr technique
    techniqueMask = SlimPtr<Technique>();
    techniqueMask->AddPass(RenderQueue::AlphaTest, CreateGLTFPipelineDesc("PBR Alpha", vShaderPbr, fShaderPbr));

    // blend pbr technique
    techniqueBlend = SlimPtr<Technique>();
    techniqueBlend->AddPass(RenderQueue::Transparent, CreateGLTFPipelineDesc("PBR Blend", vShaderPbr, fShaderPbr));
}

GLTFModel GLTFAssetManager::Load(CommandBuffer* commandBuffer, const std::string& path) {
    std::string name = slim::filesystem::path(path).filename().u8string();
    std::string base = slim::filesystem::path(path).parent_path().u8string();

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    std::string err;
    std::string warn;
    std::cout << "[LoadModel] Loading model: " << name << std::endl;
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

    GLTFModel result;

    std::cout << "[LoadModel] Loading samplers" << std::endl;
    LoadSamplers(result, model);

    std::cout << "[LoadModel] Loading images" << std::endl;
    LoadImages(result, model, commandBuffer, base);

    std::cout << "[LoadModel] Loading materials" << std::endl;
    LoadMaterials(result, model);

    std::cout << "[LoadModel] Loading meshes" << std::endl;
    LoadMeshes(result, model, commandBuffer);

    std::cout << "[LoadModel] Loading nodes" << std::endl;
    LoadNodes(result, model);

    std::cout << "[LoadModel] Loading scenes" << std::endl;
    LoadScenes(result, model);

    return result;
}

void GLTFAssetManager::LoadSamplers(GLTFModel &result, const tinygltf::Model &model) {
    for (const auto& sampler : model.samplers) {
        SamplerDesc desc;

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
        SamplerDesc desc;
        result.samplers.push_back(new Sampler(device, desc));
    }
}

void GLTFAssetManager::LoadImages(GLTFModel &result, const tinygltf::Model& model, CommandBuffer *commandBuffer, const std::string& basedir) {
    for (const auto& image : model.images) {
        std::string filepath = basedir + "/" + image.uri;
        result.images.push_back(TextureLoader::Load2D(commandBuffer, filepath));
    }
}

void GLTFAssetManager::LoadMaterials(GLTFModel &result, const tinygltf::Model &model) {
    for (const auto& material : model.materials) {

        // create new material
        SmartPtr<Material> mat;
        if (material.alphaMode == "OPAQUE") {
            mat = SlimPtr<Material>(device, techniqueOpaque);
        }
        else if (material.alphaMode == "MASK") {
            mat = SlimPtr<Material>(device, techniqueMask);
        }
        else if (material.alphaMode == "BLEND") {
            mat = SlimPtr<Material>(device, techniqueBlend);
        }

        MaterialFactors factors = {};

        // pbr metallic roughness
        const auto pbrMetallicRoughness = material.pbrMetallicRoughness;
        factors.baseColorFactor.x = pbrMetallicRoughness.baseColorFactor[0];
        factors.baseColorFactor.y = pbrMetallicRoughness.baseColorFactor[1];
        factors.baseColorFactor.z = pbrMetallicRoughness.baseColorFactor[2];
        factors.baseColorFactor.w = pbrMetallicRoughness.baseColorFactor[3];

        // base color texture
        const auto& baseColorTexture = pbrMetallicRoughness.baseColorTexture;
        if (baseColorTexture.index >= 0) {
            int32_t texture = baseColorTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.baseColorTexCoord = baseColorTexture.texCoord;
            mat->SetTexture("BaseColorTexture", result.images[image], result.samplers[sampler]);
        }

        // metallic roughness
        factors.metallicFactor = pbrMetallicRoughness.metallicFactor;
        factors.roughnessFactor = pbrMetallicRoughness.roughnessFactor;
        const auto& roughnessTexture = pbrMetallicRoughness.metallicRoughnessTexture;
        if (roughnessTexture.index >= 0) {
            int32_t texture = roughnessTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.metallicRoughnessTexCoord = roughnessTexture.texCoord;
            mat->SetTexture("MetallicRoughnessTexture", result.images[image], result.samplers[sampler]);
        }

        // normal texture
        const auto& normalTexture = material.normalTexture;
        factors.normalTexScale = normalTexture.scale;
        if (normalTexture.index >= 0) {
            int32_t texture = normalTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.normalTexCoord = normalTexture.texCoord;
            mat->SetTexture("NormalTexture", result.images[image], result.samplers[sampler]);
        }

        // occlusion texture
        const auto& occlusionTexture = material.occlusionTexture;
        factors.occlusionFactor = occlusionTexture.strength;
        if (occlusionTexture.index >= 0) {
            int32_t texture = occlusionTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.occlusionTexCoord = occlusionTexture.texCoord;
            mat->SetTexture("OcclusionTexture", result.images[image], result.samplers[sampler]);
        }

        // emissive texture
        const auto& emissiveTexture = material.emissiveTexture;
        factors.emissiveFactor.x = material.emissiveFactor[0];
        factors.emissiveFactor.y = material.emissiveFactor[1];
        factors.emissiveFactor.z = material.emissiveFactor[2];
        if (emissiveTexture.index >= 0) {
            int32_t texture = emissiveTexture.index;
            int32_t image = model.textures[texture].source;
            int32_t sampler = model.textures[texture].sampler;
            if (sampler < 0) sampler = result.samplers.size() - 1;  // default sampler
            factors.emissiveTexCoord = emissiveTexture.texCoord;
            mat->SetTexture("EmissiveTexture", result.images[image], result.samplers[sampler]);
        }

        // material factors
        mat->SetUniform("MaterialFactors", factors);

        result.materials.push_back(mat);
    }
}

void GLTFAssetManager::LoadMeshes(GLTFModel &result, const tinygltf::Model& model, CommandBuffer *commandBuffer) {
    for (const auto& mesh : model.meshes) {
        result.meshes.push_back(GLTFMesh { });
        GLTFMesh& gltfmesh = result.meshes.back();
        for (const auto& primitive : mesh.primitives) {
            std::vector<GLTFVertex> vertices;
            std::vector<uint32_t> indices;

            constexpr bool verbose = false;

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

            GLTFPrimitive prim;

            prim.mesh = SlimPtr<Mesh>();
            prim.mesh->SetVertexAttrib(commandBuffer, vertices, 0);
            prim.vertexCount = vertices.size();

            // index
            if (!indices.empty()) {
                prim.mesh->SetIndexAttrib(commandBuffer, indices);
                prim.indexCount = indices.size();
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

void GLTFAssetManager::LoadNodes(GLTFModel &result, const tinygltf::Model &model) {
    for (const auto &node : model.nodes) {

        // create new node
        auto snode = manager->Create<Scene>(node.name);
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
                if (primitive.indexCount > 0) {
                    snode->AddDraw(primitive.mesh, primitive.material, DrawIndexed { primitive.indexCount, 1, 0, 0, 0 });
                } else {
                    snode->AddDraw(primitive.mesh, primitive.material, DrawCommand { primitive.vertexCount, 1, 0, 0 });
                }
            }
        }

    }

    // initialize scene node hierarchy
    for (uint32_t i = 0; i < model.nodes.size(); i++) {
        Scene* snode = result.nodes[i];
        for (int child : model.nodes[i].children) {
            snode->AddChild(result.nodes[child]);
        }
    }

}

void GLTFAssetManager::LoadScenes(GLTFModel& result, const tinygltf::Model& model) {
    for (const auto &scene : model.scenes) {
        result.scenes.push_back(GLTFScene { });
        GLTFScene& scn = result.scenes.back();
        scn.name = scene.name;
        for (int node : scene.nodes) {
            scn.roots.push_back(result.nodes[node]);
        }
    }
}

void GLTFAssetManager::ReadVertexPosition(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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
        vertices[i].pos = glm::vec3(p[0], p[1], p[2]);
    }
}

void GLTFAssetManager::ReadVertexNormal(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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

void GLTFAssetManager::ReadVertexTangent(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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

void GLTFAssetManager::ReadVertexTexCoord0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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

void GLTFAssetManager::ReadVertexTexCoord1(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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

void GLTFAssetManager::ReadVertexColor0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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

void GLTFAssetManager::ReadVertexJoints0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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

void GLTFAssetManager::ReadVertexWeights0(std::vector<GLTFVertex>& vertices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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

void GLTFAssetManager::ReadIndices(std::vector<uint32_t>& indices, const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
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
