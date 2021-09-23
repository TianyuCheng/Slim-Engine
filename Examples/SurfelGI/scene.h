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

private:
    void PrepareScene();
    void PrepareTransformBuffer();

private:
    SmartPtr<Device>                  device;

public:
    // scene/models
    SmartPtr<scene::Builder>          builder;
    gltf::Model                       model;

    // other scene data
    SmartPtr<Buffer>                  transformBuffer;
    std::vector<Image*>               images;
    std::vector<Sampler*>             samplers;
};
