#include "scene.h"

MainScene::MainScene(Device* device) : device(device) {
    PrepareScene();
    PrepareCamera();
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
    model.Load(builder, GetUserAsset("Scenes/Sponza/glTF/Sponza.gltf"));

    #ifdef MINUSCALE_SCENE
    model.GetScene(0)->Scale(0.01, 0.01, 0.01);
    model.GetScene(0)->ApplyTransform();
    #endif

    // adding bounding box for procedural hit
    #ifdef MINUSCALE_SCENE
    builder->AddAABB(BoundingBox(glm::vec3(-300, -300, -300), glm::vec3(+300, +300, +300)));
    #else
    builder->AddAABB(BoundingBox(glm::vec3(-3000, -3000, -3000), glm::vec3(+3000, +3000, +3000)));
    #endif

    // create scene
    builder->Build();

    // prepare images
    for (auto& image : model.images) images.push_back(image);

    // prepare samplers
    for (auto& sampler : model.samplers) samplers.push_back(sampler);
}

void MainScene::PrepareCamera() {
    // camera
    camera = SlimPtr<Flycam>("camera");
    #ifdef MINUSCALE_SCENE
    camera->SetWalkSpeed(1.0f);
    camera->LookAt(glm::vec3(0.03, 1.35, 0.0), glm::vec3(0.0, 1.35, 0.0), glm::vec3(0.0, 1.0, 0.0));
    #else
    camera->SetWalkSpeed(100.0f);
    camera->LookAt(glm::vec3(3.0, 135.0, 0.0), glm::vec3(0.0, 135.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    #endif
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
