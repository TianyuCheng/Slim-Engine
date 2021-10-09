#include "scene.h"

MainScene::MainScene(Device* device) : device(device) {
    PrepareScene();
    PrepareCamera();
    PrepareTransformBuffer();
    PrepareLightBuffer();
    PrepareCameraBuffer();
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
    camera->SetWalkSpeed(100.0f);
    camera->LookAt(glm::vec3(3.0, 135.0, 0.0), glm::vec3(0.0, 135.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
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

void MainScene::PrepareLightBuffer() {
    dirLight = DirectionalLight {
        glm::vec4(0.0, -1.0, 0.0, 0.0), // direction
        glm::vec4(1.0, 1.0, 1.0, 1.0),  // light color
    };

    lightBuffer = SlimPtr<Buffer>(device, sizeof(LightInfo),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    lightBuffer->SetName("Light Buffer");
}

void MainScene::PrepareCameraBuffer() {
    cameraBuffer = SlimPtr<Buffer>(device, sizeof(CameraInfo),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    cameraBuffer->SetName("Camera Buffer");
}

void AddScenePreparePass(RenderGraph& renderGraph, MainScene* scene) {
    // compile
    auto preparePass = renderGraph.CreateComputePass("prepare");
    preparePass->SetStorage(scene->lightResource, RenderGraph::STORAGE_WRITE_BIT);
    preparePass->SetStorage(scene->cameraResource, RenderGraph::STORAGE_WRITE_BIT);

    // execute
    preparePass->Execute([=](const RenderInfo& info) {
        scene->UpdateLight(info.commandBuffer);
        scene->UpdateCamera(info.commandBuffer);
    });
}
