#include "model.h"
#include "ghc/filesystem.hpp"

// uint32_t SizeOfGLTFComponentType(uint32_t type) {
//     switch (type) {
//         case TINYGLTF_COMPONENT_TYPE_BYTE:
//         case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
//             return sizeof(char);
//         case TINYGLTF_COMPONENT_TYPE_SHORT:
//         case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
//             return sizeof(short);
//         case TINYGLTF_COMPONENT_TYPE_INT:
//         case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
//             return sizeof(int);
//         case TINYGLTF_COMPONENT_TYPE_FLOAT:
//             return sizeof(float);
//         case TINYGLTF_COMPONENT_TYPE_DOUBLE:
//             return sizeof(double);
//     }
//     assert(!!!"invalid component type");
//     return 0;
// }
//
// int32_t IndexOfVertexAttrib(const std::string &name) {
//     if (name == "POSITION")   return 0;
//     if (name == "NORMAL")     return 1;
//     if (name == "TEXCOORD_0") return 2;
//     return -1;
// }
//
// void InitImages(Context *context, GLTFModel &result, const tinygltf::Model &model, const std::string &path) {
//     // initialize textures
//     GLTFModel* pResult = &result;
//     std::string basedir = ghc::filesystem::path(path).parent_path();
//     context->Execute([=](CommandBuffer *commandBuffer) {
//         for (const auto &image : model.images) {
//             std::string filepath = basedir + "/" + image.uri;
//             pResult->images.push_back(TextureLoader::Load2D(commandBuffer, filepath));
//         }
//     });
// }
//
// void InitSamplers(Context *context, GLTFModel &result, const tinygltf::Model &model) {
//     for (const auto& sampler : model.samplers) {
//         SamplerDesc desc;
//
//         // min filter + mip filter
//         if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) desc.MinFilter(VK_FILTER_NEAREST);
//         if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR)  desc.MinFilter(VK_FILTER_LINEAR);
//         if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
//             desc.MinFilter(VK_FILTER_NEAREST);
//             desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST);
//         }
//         if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
//             desc.MinFilter(VK_FILTER_NEAREST);
//             desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
//         }
//         if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
//             desc.MinFilter(VK_FILTER_LINEAR);
//             desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST);
//         }
//         if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
//             desc.MinFilter(VK_FILTER_LINEAR);
//             desc.MipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
//         }
//
//         // max filter
//         if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) desc.MagFilter(VK_FILTER_NEAREST);
//         if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR)  desc.MagFilter(VK_FILTER_LINEAR);
//
//         // wrap S
//         VkSamplerAddressMode addrS;
//         if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)   addrS = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//         if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) addrS = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
//         if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_REPEAT)          addrS = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
//
//         // wrap T
//         VkSamplerAddressMode addrT;
//         if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)   addrT = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//         if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) addrT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
//         if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_REPEAT)          addrT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
//
//         result.samplers.push_back(new Sampler(context, desc));
//     }
//
//     if (result.samplers.size() == 0) {
//         SamplerDesc desc;
//         result.samplers.push_back(new Sampler(context, desc));
//     }
// }
//
// void InitMaterials(Context *context, GLTFModel &result, const tinygltf::Model &model) {
//     for (const auto& material : model.materials) {
//         result.materials.push_back(new Material(context, result.technique));
//         Material* targetMaterial = result.materials.back();
//         uint32_t textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
//         int imageIndex = model.textures[textureIndex].source;
//         int samplerIndex = model.textures[textureIndex].sampler;
//         if (samplerIndex < 0) samplerIndex = 0;
//         targetMaterial->SetTexture("BaseColorTexture",
//                 result.images[imageIndex],
//                 result.samplers[samplerIndex]);
//     }
// }
//
// void InitBuffers(Context *context, GLTFModel &result, const tinygltf::Model &model) {
//     // initialize buffers
//     GLTFModel* pResult = &result;
//     context->Execute([=](CommandBuffer *commandBuffer) {
//         for (const auto &buffer : model.buffers) {
//             // GLTF's buffer usually contains more than 1 type of data, i.e. index buffer/vertex buffer/bone information, etc
//             Buffer *buf = new Buffer(context, buffer.data.size(),
//                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
//                 VMA_MEMORY_USAGE_GPU_ONLY);
//
//             commandBuffer->CopyDataToBuffer(buffer.data, buf);
//
//             pResult->buffers.push_back(buf);
//         }
//     });
// }
//
// void InitMeshes(GLTFModel &result, const tinygltf::Model &model) {
//     for (const auto& mesh : model.meshes) {
//         result.meshes.push_back(GLTFMesh { });
//         GLTFMesh &targetMesh = result.meshes.back();
//         for (const auto &primitive : mesh.primitives) {
//
//             // make sure it is triangle-based, otherwise it is not supported
//             assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
//
//             targetMesh.primitives.push_back(new Mesh());
//             Mesh *mesh = targetMesh.primitives.back();
//
//             // index buffer
//             const auto& indexAccessor = model.accessors[primitive.indices];
//             const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
//             mesh->SetIndexAttrib(result.buffers[indexBufferView.buffer], indexBufferView.byteOffset + indexAccessor.byteOffset,
//                     SizeOfGLTFComponentType(indexAccessor.componentType) == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
//
//             // vertex buffers
//             for (const auto& kv : primitive.attributes) {
//                 const auto& vertexAccessor = model.accessors[kv.second];
//                 const auto& vertexBufferView = model.bufferViews[vertexAccessor.bufferView];
//                 int32_t vertexIndex = IndexOfVertexAttrib(kv.first);
//                 if (vertexIndex >= 0) {
//                     mesh->SetVertexAttrib(result.buffers[vertexBufferView.buffer], vertexBufferView.byteOffset + vertexAccessor.byteOffset, vertexIndex);
//                 }
//             }
//
//             targetMesh.submeshes.push_back(Submesh {});
//             Submesh &submesh = targetMesh.submeshes.back();
//             submesh.mesh = mesh;
//             submesh.indexOffset = 0;
//             submesh.vertexOffset = 0;
//             submesh.instanceCount = 1;
//             submesh.indexCount = model.accessors[primitive.indices].count;
//
//             targetMesh.materials.push_back(primitive.material);
//
//         } // end of mesh primitives
//     } // end of model meshes
// }
//
// void InitNodes(GLTFModel &result, const tinygltf::Model &model) {
//     for (const auto &node : model.nodes) {
//         // initialize scene node
//         result.nodes.push_back(new SceneNode(node.name));
//         SceneNode* snode = result.nodes.back();
//
//         // initialize scene node transform
//         if (node.matrix.size()) {
//             glm::mat4 matrix;
//             for (int i = 0; i < 4; i++)
//                 for (int j = 0; j < 4; j++)
//                     matrix[i][j] = node.matrix[i * 4 + j];
//             snode->SetTransform(matrix);
//         }
//
//         if (node.rotation.size()) {
//             snode->Rotate(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
//         }
//
//         if (node.scale.size()) {
//             snode->Translate(node.scale[0], node.scale[1], node.scale[2]);
//         }
//
//         if (node.translation.size()) {
//             snode->Translate(node.translation[0], node.translation[1], node.translation[2]);
//         }
//
//         #if 1
//         if (node.mesh >= 0) {
//             Material* material = result.materials[result.meshes[node.mesh].materials[0]];
//             snode->SetMesh(result.meshes[node.mesh].submeshes[0]);
//             snode->SetMaterial(material);
//         }
//         #endif
//     }
//
//     // initialize scene node hierarchy
//     for (uint32_t i = 0; i < model.nodes.size(); i++) {
//         SceneNode* snode = result.nodes[i];
//         for (int child : model.nodes[i].children) {
//             snode->AddChild(result.nodes[child]);
//         }
//     }
// }
//
// void InitScenes(GLTFModel &result, const tinygltf::Model &model) {
//     for (const auto& scene : model.scenes) {
//         result.scenes.push_back(new SceneGraph());
//         SceneGraph* targetScene = result.scenes.back();
//         for (auto index : scene.nodes) {
//             targetScene->AddRootNode(result.nodes[index]);
//         }
//         targetScene->Init();
//     }
// }
//
// void LoadModel(Context *context, GLTFModel &result, const std::string &path) {
//     tinygltf::Model model;
//     tinygltf::TinyGLTF loader;
//     std::string err;
//     std::string warn;
//
//     bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path, tinygltf::REQUIRE_ALL);
//
//     if (!warn.empty()) { std::cout << "Warn: " << warn << std::endl; }
//     if (!err.empty()) { std::cout << "Err: " << err << std::endl; }
//     if (!ret) { std::cout << "Failed to parse glTF" << std::endl; exit(1); }
//
//     // create vertex and fragment shaders
//     result.vShader = new spirv::VertexShader(context, "main", "shaders/gltf.vert.spv");
//     result.fShader = new spirv::FragmentShader(context, "main", "shaders/gltf.frag.spv");
//
//     // create technique
//     result.technique = SlimPtr<Technique>();
//     result.technique->AddPass(RenderQueue::Opaque,
//         GraphicsPipelineDesc()
//             .SetName("gltf")
//             .AddVertexBinding(0, sizeof(GLTFVertex::position),  VK_VERTEX_INPUT_RATE_VERTEX, { { 0, VK_FORMAT_R32G32B32_SFLOAT, 0 } })
//             .AddVertexBinding(1, sizeof(GLTFVertex::normal),    VK_VERTEX_INPUT_RATE_VERTEX, { { 1, VK_FORMAT_R32G32B32_SFLOAT, 0 } })
//             .AddVertexBinding(2, sizeof(GLTFVertex::texcoord0), VK_VERTEX_INPUT_RATE_VERTEX, { { 2, VK_FORMAT_R32G32_SFLOAT,    0 } })
//             .SetVertexShader(result.vShader)
//             .SetFragmentShader(result.fShader)
//             .SetCullMode(VK_CULL_MODE_BACK_BIT)
//             .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
//             .SetDepthTest(VK_COMPARE_OP_LESS)
//             .SetPipelineLayout(PipelineLayoutDesc()
//                 .AddPushConstant("Xform", 0, 2 * sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                 .AddBinding("Camera",           0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
//                 .AddBinding("BaseColorTexture", 1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
//             ));
//
//     // initialize images
//     InitImages(context, result, model, path);
//
//     // initialize samplers
//     InitSamplers(context, result, model);
//
//     // initialize buffers
//     InitBuffers(context, result, model);
//
//     context->WaitIdle();
//
//     // initialize materials
//     InitMaterials(context, result, model);
//
//     // initialize meshes
//     InitMeshes(result, model);
//
//     // initialize scene nodes
//     InitNodes(result, model);
//
//     // initialize scenes
//     InitScenes(result, model);
//
//     // default scene
//     result.defaultScene = result.scenes[model.defaultScene];
//
//     for (SceneNode* root : *result.defaultScene) {
//         root->Rotate(glm::vec3(0.0, 0.0, 1.0), -M_PI / 2.0);
//         root->Rotate(glm::vec3(1.0, 0.0, 0.0), M_PI / 2.0);
//         root->Translate(0.0, 0.0, -10.0);
//     }
// }
