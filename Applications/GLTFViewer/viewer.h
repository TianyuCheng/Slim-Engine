#ifndef GLTFVIEWER_VIEWER_H
#define GLTFVIEWER_VIEWER_H

#include <slim/slim.hpp>
#include "config.h"
#include "gizmo.h"
#include "skybox.h"

using namespace slim;

class GLTFViewer {
public:
    explicit GLTFViewer();
    virtual ~GLTFViewer();

    void Run();

private:
    void InitContext();
    void InitDevice();
    void InitWindow();
    void InitInput();
    void InitCamera();
    void InitGizmo();
    void InitSkybox();
    void InitLUT();
    void InitPBR();
    void InitSampler();
    void LoadModel();
    void ProcessModel(gltf::Model* model);

private:
    SmartPtr<Context>               context;
    SmartPtr<Device>                device;
    SmartPtr<Window>                window;
    SmartPtr<Input>                 input;
    SmartPtr<Gizmo>                 gizmo;
    SmartPtr<Skybox>                skybox;
    SmartPtr<Arcball>               camera;
    SmartPtr<Time>                  time;

    SmartPtr<GPUImage>              dfglut;
    SmartPtr<Sampler>               diffuseSampler;
    SmartPtr<Sampler>               specularSampler;
    SmartPtr<Sampler>               dfglutSampler;

    SmartPtr<scene::Builder>        builder;
    SmartPtr<scene::Node>           root;
    SmartPtr<gltf::Model>           model;

    SmartPtr<spirv::VertexShader>   vShaderPbr;
    SmartPtr<spirv::FragmentShader> fShaderPbr;
    SmartPtr<Technique>             techniqueOpaque;
    SmartPtr<Technique>             techniqueMask;
    SmartPtr<Technique>             techniqueBlend;
};

#endif // GLTFVIEWER_VIEWER_H
