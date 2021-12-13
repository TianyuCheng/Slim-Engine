#ifndef SLIM_EXAMPLE_SCENE_H
#define SLIM_EXAMPLE_SCENE_H

#include <slim/slim.hpp>
using namespace slim;

#include "shaders/common.h"

struct BilateralInfo {
    glm::vec2 singleStepOffset;
    float distanceNormalizationFactor;
    float padding0;
};

class Scene {
private:
    SmartPtr<Device>         device;
    gltf::Model              model;
    SmartPtr<scene::Builder> geometryBuilder;

public:
    // scene data
    SmartPtr<scene::Builder> builder;
    SmartPtr<Flycam>         camera;
    std::vector<Image*>      images;
    std::vector<Sampler*>    samplers;
    std::vector<LightInfo>   lights;
    SkyInfo                  sky;

    SmartPtr<Buffer>         skyBuffer;
    SmartPtr<Buffer>         lightBuffer;
    SmartPtr<Buffer>         cameraBuffer;
    SmartPtr<Buffer>         materialBuffer;
    SmartPtr<Buffer>         instanceBuffer;
    SmartPtr<Buffer>         frameInfoBuffer;
    SmartPtr<Buffer>         lightXformBuffer;
    SmartPtr<Buffer>         bilateralBuffer;

    // light geometry
    SmartPtr<scene::Mesh>    coneMesh;
    SmartPtr<scene::Mesh>    sphereMesh;
    SmartPtr<scene::Mesh>    cylinderMesh;

    // surfel data
    SmartPtr<Buffer>         surfelBuffer;
    SmartPtr<Buffer>         surfelLiveBuffer;  // stack for living and free surfels
    SmartPtr<Buffer>         surfelDataBuffer;  // raw surfel data
    SmartPtr<Buffer>         surfelGridBuffer;  // surfel grid acceleration structure
    SmartPtr<Buffer>         surfelCellBuffer;  // offset + count for grid cell into surfelLiveBuffer
    SmartPtr<Buffer>         surfelStatBuffer;  // global surfel status
    SmartPtr<Buffer>         surfelRayDirBuffer;// surfel ray visualize
    SmartPtr<GPUImage>       surfelDepthImage;
    SmartPtr<GPUImage>       surfelRayGuideImage;

    // control
    float                    walkSpeed;
    DebugControl             debugControl;

    explicit Scene(Device* device);

    float Near() const;
    float Far() const;

    void ResetSurfels();
    void PauseSurfels();
    void ResumeSurfels();

private:
    void InitScene();
    void InitFrame();
    void InitCamera();
    void InitLights();
    void InitSurfels();
    void InitGeometry();
    void InitBilateral();
};

#endif // SLIM_EXAMPLE_SCENE_H
