#include "meshrenderer.h"

using namespace slim;

MeshRenderer::MeshRenderer(const RenderInfo &info) : info(info) {
}

MeshRenderer::~MeshRenderer() {
}

void MeshRenderer::Draw(Camera *camera, const View<Drawable>& drawables) {
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
    if (modelData.size() == 0) return;

    // camera uniform + model uniform
    auto cameraUniform = renderFrame->RequestUniformBuffer(cameraData);
    auto modelUniform = renderFrame->RequestUniformBuffer(modelData);

    // draw
    #if 1
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
        if (scene->indexed) {
            commandBuffer->DrawIndexed(scene->draw.count, 1, scene->draw.first, scene->draw.offset, 0);
        } else {
            commandBuffer->Draw(scene->draw.count, 1, scene->draw.first, 0);
        }

        index++;
    }
    #endif
}
