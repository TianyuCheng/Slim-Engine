#include "meshrenderer.h"

using namespace slim;

MeshRenderer::MeshRenderer(const RenderInfo &info) : info(info) {
}

MeshRenderer::~MeshRenderer() {
}

void MeshRenderer::Draw(Camera *camera, const View<Drawable>& drawables) {
    if (drawables.empty()) {
        return;
    }

    RenderPass* renderPass = info.renderPass;
    RenderFrame* renderFrame = info.renderFrame;
    CommandBuffer* commandBuffer = info.commandBuffer;

    CameraData cameraData = {
        camera->GetView(),
        camera->GetProjection()
    };

    std::vector<ModelData> modelData;
    std::vector<Material*> materials;

    // prepare model transforms
    for (const Drawable& drawable : drawables) {
        glm::mat4 mv = cameraData.view * drawable.node->GetTransform().LocalToWorld();
        modelData.push_back(ModelData {
            drawable.node->GetTransform().LocalToWorld(),
            glm::mat3(glm::transpose(glm::inverse(mv)))
        });
        materials.push_back(drawable.material);
    }

    // nothing to draw
    if (modelData.empty()) return;

    // camera uniform + model uniform
    auto cameraUniform = renderFrame->RequestUniformBuffer(cameraData);
    auto modelUniform = renderFrame->RequestUniformBuffer(modelData);

    // draw
    uint32_t index = 0;
    for (const Drawable& drawable : drawables) {
        drawable.mesh->Bind(commandBuffer);

        uint32_t techniqueIndex = drawable.material->QueueIndex(drawable.queue);
        drawable.material->Bind(techniqueIndex, commandBuffer, renderFrame, renderPass);

        // bind
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), drawable.material->Layout(techniqueIndex));
        descriptor->SetUniform("Camera", cameraUniform);
        descriptor->SetDynamic("Model", modelUniform, sizeof(ModelData));
        descriptor->SetDynamicOffset("Model", index * sizeof(ModelData));
        commandBuffer->BindDescriptor(descriptor);

        // draw
        const auto& draw = drawable.drawCommand;
        if (std::holds_alternative<DrawCommand>(draw)) {
            const auto& command = std::get<VkDrawIndirectCommand>(draw);
            commandBuffer->Draw(command.vertexCount, command.instanceCount, command.firstVertex, command.firstInstance);
        } else {
            const auto& command = std::get<VkDrawIndexedIndirectCommand>(draw);
            commandBuffer->DrawIndexed(command.indexCount, command.instanceCount, command.firstIndex, command.vertexOffset, command.firstInstance);
        }

        index++;
    }
}
