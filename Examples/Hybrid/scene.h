#ifndef SLIM_EXAMPLE_SCENE_H
#define SLIM_EXAMPLE_SCENE_H

#include <slim/slim.hpp>
#include "config.h"
#include "light.h"

using namespace slim;

struct ObjectProperties {
    uint32_t instanceID;
    uint32_t baseColorTextureID;
    uint32_t baseColorSamplerID;
};

struct LightInfo {
    glm::vec4 direction;
    glm::vec4 color;
};

struct CameraInfo {
    glm::mat4 V;
    glm::mat4 P;
};

class MainScene {
public:
    MainScene(Device* device);

    scene::Node* GetRootNode() const {
        return model.GetScene(0);
    }

    accel::AccelStruct* GetTlas() const {
        return builder->GetAccelBuilder()->GetTlas();
    }

    void UpdateCamera(CommandBuffer* commandBuffer) {
        CameraInfo info;
        info.P = camera->GetProjection();
        info.V = camera->GetView();
        commandBuffer->CopyDataToBuffer(info, cameraBuffer);
    }

    void UpdateLight(CommandBuffer* commandBuffer) {
        LightInfo info;
        info.color = dirLight.color;
        info.direction = dirLight.direction;
        commandBuffer->CopyDataToBuffer(info, lightBuffer);
    }

private:
    void PrepareScene();
    void PrepareCamera();
    void PrepareTransformBuffer();
    void PrepareLightBuffer();
    void PrepareCameraBuffer();

private:
    SmartPtr<Device>                  device;

public:
    // scene/models
    SmartPtr<scene::Builder>          builder;
    SmartPtr<Flycam>                  camera;
    gltf::Model                       model;

    // light
    DirectionalLight                  dirLight;

    // other scene data
    SmartPtr<Buffer>                  transformBuffer;
    std::vector<Image*>               images;
    std::vector<Sampler*>             samplers;

    SmartPtr<Buffer>                  cameraBuffer;
    RenderGraph::Resource*            cameraResource;

    SmartPtr<Buffer>                  lightBuffer;
    RenderGraph::Resource*            lightResource;
};

void AddScenePreparePass(RenderGraph& renderGraph, MainScene* scene);

#endif // SLIM_EXAMPLE_SCENE_H
