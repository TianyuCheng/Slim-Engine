#include "scene.h"

MainScene::MainScene(Device* device) : device(device) {
    PrepareScene();
    PrepareTransformBuffer();
}

void MainScene::PrepareScene() {
    // model loading
    builder = SlimPtr<scene::Builder>(device);

    // enable ray tracing builder and acceleration structure compaction
    #ifdef ENABLE_RAY_TRACING
    builder->EnableRayTracing();
    builder->GetAccelBuilder()->EnableCompaction();
    #endif

    // load model
    model.Load(builder, ToAssetPath("Scenes/Sponza/glTF/Sponza.gltf"));

    // create scene
    builder->Build();

    // prepare images
    for (auto& image : model.images) images.push_back(image);

    // prepare samplers
    for (auto& sampler : model.samplers) samplers.push_back(sampler);
}

void MainScene::PrepareTransformBuffer() {
    // initialize transform data
    std::vector<glm::mat4> transforms;
    builder->ForEachInstance<glm::mat4>(transforms,
        [&](glm::mat4 &transform, scene::Node *node, scene::Mesh *, scene::Material *, uint32_t) {
        transform = node->GetTransform().LocalToWorld();
    });

    // initialize transform buffer
    uint32_t bufferSize = transforms.size() * sizeof(transforms[0]);
    VkBufferUsageFlags transformBufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    #ifdef ENABLE_RAY_TRACING
                                            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    #endif
    ;
    transformBuffer = SlimPtr<Buffer>(device, bufferSize, transformBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);

    // copy transforms to buffer
    device->Execute([&](CommandBuffer* commandBuffer) {
        commandBuffer->CopyDataToBuffer(transforms, transformBuffer);
    });
}
