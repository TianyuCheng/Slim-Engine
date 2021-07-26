#include "material.h"

// Material* CreateMaterial(Shader* vShader, Shader *fShader) {
//     auto material = new Material("material", Material::Opaque);
//     material->GetPipelineDesc()
//         .SetName("textured")
//         .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
//             { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
//             { 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, texcoord) },
//          })
//         .SetVertexShader(vShader)
//         .SetFragmentShader(fShader)
//         .SetCullMode(VK_CULL_MODE_BACK_BIT)
//         .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
//         .SetDepthTest(VK_COMPARE_OP_LESS)
//         .SetPipelineLayout(PipelineLayoutDesc()
//             .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
//             .AddBinding("Albedo", 1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));
//
//     // This function is for setting up node specific uniform, like model transform
//     material->PrepareSceneNode = [=](const RenderInfo &info) {
//         auto mvp = info.camera->GetViewProjection() * info.sceneNode->GetTransform().LocalToWorld();
//         auto descriptor = SlimPtr<Descriptor>(info.renderFrame, info.material->GetPipeline());
//         descriptor->SetUniform("Camera", info.renderFrame->RequestUniformBuffer(mvp));
//         info.commandBuffer->BindDescriptor(descriptor);
//     };
//
//     return material;
// }
