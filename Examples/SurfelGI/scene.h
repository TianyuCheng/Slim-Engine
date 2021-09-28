#include <slim/slim.hpp>
#include "config.h"
#include "light.h"

using namespace slim;

struct ObjectProperties {
    uint32_t instanceID;
    uint32_t baseColorTextureID;
    uint32_t baseColorSamplerID;
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

    float GetNear() const {
        #ifdef MINUSCALE_SCENE
        return 0.001;
        #else
        return 0.1;
        #endif
    }

    float GetFar() const {
        #ifdef MINUSCALE_SCENE
        return 30.0;
        #else
        return 3000.0;
        #endif
    }

private:
    void PrepareScene();
    void PrepareCamera();
    void PrepareTransformBuffer();

private:
    SmartPtr<Device>                  device;

public:
    // scene/models
    SmartPtr<scene::Builder>          builder;
    SmartPtr<Flycam>                  camera;
    gltf::Model                       model;

    // other scene data
    SmartPtr<Buffer>                  transformBuffer;
    std::vector<Image*>               images;
    std::vector<Sampler*>             samplers;
};
