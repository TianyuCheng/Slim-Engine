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
        modelData.push_back(ModelData {
            drawable.node->GetTransform().LocalToWorld(),
            glm::transpose(cameraData.view * drawable.node->GetTransform().LocalToWorld())
        });
        materials.push_back(drawable.node->GetMaterial());
    }

    // nothing to draw
    if (modelData.empty()) return;

    // camera uniform + model uniform
    auto cameraUniform = renderFrame->RequestUniformBuffer(cameraData);
    auto modelUniform = renderFrame->RequestUniformBuffer(modelData);

    // draw
    uint32_t index = 0;
    for (const Drawable& drawable : drawables) {

        Scene* scene = drawable.node;
        scene->GetMesh()->Bind(commandBuffer);

        uint32_t techniqueIndex = scene->GetMaterial()->QueueIndex(drawable.queue);
        scene->GetMaterial()->Bind(techniqueIndex, commandBuffer, renderFrame, renderPass);

        // bind
        auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), scene->GetMaterial()->Layout(techniqueIndex));
        descriptor->SetUniform("Camera", cameraUniform);
        descriptor->SetDynamic("Model", modelUniform, sizeof(ModelData));
        descriptor->SetDynamicOffset("Model", index * sizeof(ModelData));
        commandBuffer->BindDescriptor(descriptor);

        // draw
        const auto& draw = scene->GetDraw();
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
