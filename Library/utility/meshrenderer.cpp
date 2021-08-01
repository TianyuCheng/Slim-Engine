#include "meshrenderer.h"

using namespace slim;

MeshRenderer::MeshRenderer(const RenderInfo &info) : info(info) {
}

MeshRenderer::~MeshRenderer() {
}

void MeshRenderer::Draw(Camera* camera, SceneFilter* filter, uint32_t first, uint32_t last) {
    // struct Camera {
    //     alignas(16) glm::mat4 view;
    //     alignas(16) glm::mat4 proj;
    // };
    //
    // struct alignas(256) Model {
    //     alignas(16) glm::mat4 model;
    //     alignas(16) glm::mat4 normal;
    // };
    //
    // RenderPass* renderPass = info.renderPass;
    // RenderFrame* renderFrame = info.renderFrame;
    // CommandBuffer* commandBuffer = info.commandBuffer;
    //
    // Camera cam = { camera->GetView(), camera->GetProjection() };
    //
    // // prepare model transforms
    // std::vector<Model> models;
    // for (const auto& kv : filter->objects) {
    //     RenderQueue queue = kv.first;
    //     if (queue >= first && queue <= last) {
    //         for (const auto& drawable : kv.second) {
    //             models.push_back(Model {
    //                 drawable.node->GetTransform().LocalToWorld(),
    //                 glm::transpose(cam.view * drawable.node->GetTransform().LocalToWorld()),
    //             });
    //         } // end of object loop
    //     }
    // } // end of queue loop
    //
    // // nothing to draw
    // if (models.size() == 0) return;
    //
    // // camera uniform + model uniform
    // auto cameraUniform = renderFrame->RequestUniformBuffer(cam);
    // auto modelUniform = renderFrame->RequestUniformBuffer(models);
    //
    // // TODO: batch this! batch this! batch this!
    //
    // // draw
    // uint32_t index = 0;
    // for (const auto& kv : filter->objects) {
    //     RenderQueue queue = kv.first;
    //     if (queue >= first && queue <= last) {
    //         for (const auto& drawable : kv.second) {
    //             Scene* scene = drawable.node;
    //             scene->GetMesh()->Bind(commandBuffer);
    //
    //             uint32_t materialQueueIndex = scene->GetMaterial()->QueueIndex(queue);
    //             scene->GetMaterial()->Bind(materialQueueIndex, commandBuffer, renderFrame, renderPass);
    //
    //             // bind
    //             auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), scene->GetMaterial()->Layout(materialQueueIndex));
    //             descriptor->SetUniform("Camera", cameraUniform);
    //             descriptor->SetDynamic("Model", modelUniform, 0, sizeof(Model));
    //             descriptor->SetDynamicOffset("Model", index * sizeof(Model));
    //             commandBuffer->BindDescriptor(descriptor);
    //
    //             // draw
    //             if (scene->indexed) {
    //                 commandBuffer->DrawIndexed(scene->draw.count, 1, scene->draw.first, scene->draw.offset, 0);
    //             } else {
    //                 commandBuffer->Draw(scene->draw.count, 1, scene->draw.first, 0);
    //             }
    //
    //             index++;
    //         } // end of object loop
    //     }
    // } // end of queue loop
}
