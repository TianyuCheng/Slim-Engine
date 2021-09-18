#include "meshrenderer.h"
#include <iostream>
#include <glm/gtx/string_cast.hpp>

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
    std::vector<scene::Material*> materials;

    // prepare model transforms
    for (const Drawable& drawable : drawables) {
        glm::mat4 M = drawable.node->GetTransform().LocalToWorld();
        glm::mat4 N = glm::transpose(glm::inverse(cameraData.view * M));
        modelData.push_back(ModelData { M, N });
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
        descriptor->SetUniformBuffer("Camera", cameraUniform);
        descriptor->SetDynamicUniformBuffer("Model", modelUniform, sizeof(ModelData));
        descriptor->SetDynamicOffset("Model", index * sizeof(ModelData));
        commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);

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
