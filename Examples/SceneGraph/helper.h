#include <slim/slim.hpp>

using namespace slim;

struct Vertex {
    glm::vec3 position;
};

// inline Material* CreateMaterial(Shader* vShader, Shader *fShader) {
//     // auto material = new Material("material", Material::Opaque);
//     // material->GetPipelineDesc()
//     //     .SetName("textured")
//     //     .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
//     //         { 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
//     //      })
//     //     .SetVertexShader(vShader)
//     //     .SetFragmentShader(fShader)
//     //     .SetCullMode(VK_CULL_MODE_BACK_BIT)
//     //     .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
//     //     .SetDepthTest(VK_COMPARE_OP_LESS)
//     //     .SetPipelineLayout(PipelineLayoutDesc()
//     //         .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
//     //         .AddBinding("Color",  1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT));
//
//     // // This function is for setting up material specific uniform, like texture
//     // material->PrepareMaterial = [=](const RenderInfo &info) {
//     //     auto color = glm::vec3(1.0, 1.0, 0.0);
//     //     auto descriptor = SlimPtr<Descriptor>(info.renderFrame, info.material->GetPipeline());
//     //     descriptor->SetUniform("Color", info.renderFrame->RequestUniformBuffer(color));
//     //     info.commandBuffer->BindDescriptor(descriptor);
//     // };
//
//     // // This function is for setting up node specific uniform, like model transform
//     // material->PrepareSceneNode = [=](const RenderInfo &info) {
//     //     auto mvp = info.camera->GetViewProjection() * info.sceneNode->GetTransform().LocalToWorld();
//     //     auto descriptor = SlimPtr<Descriptor>(info.renderFrame, info.material->GetPipeline());
//     //     descriptor->SetUniform("Camera", info.renderFrame->RequestUniformBuffer(mvp));
//     //     info.commandBuffer->BindDescriptor(descriptor);
//     // };
//
//     return material;
// }

inline Mesh* CreateMesh(Context *context) {
    Mesh* mesh = new Mesh();
    context->Execute([=](CommandBuffer *commandBuffer) {
        // prepare vertex data
        std::vector<glm::vec3> positions = {
            // position
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f),

            // position
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),
        };

        // prepare index data
        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,
            4, 7, 6, 6, 5, 4,
            3, 7, 4, 4, 0, 3,
            1, 5, 6, 6, 2, 1,
            6, 7, 3, 3, 2, 6,
            4, 5, 1, 1, 0, 4
        };

        mesh->SetVertexAttrib(commandBuffer, positions, 0);
        mesh->SetIndexAttrib(commandBuffer, indices);
    });
    return mesh;
}
