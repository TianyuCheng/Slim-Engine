#ifndef GLTFVIEWER_VIEWER_H
#define GLTFVIEWER_VIEWER_H

#include <slim/slim.hpp>
#include "config.h"
#include "model.h"
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
    void InitSampler();
    void LoadModel();

private:
    SmartPtr<Context>          context;
    SmartPtr<Device>           device;
    SmartPtr<Window>           window;
    SmartPtr<Input>            input;
    SmartPtr<Gizmo>            gizmo;
    SmartPtr<Skybox>           skybox;
    SmartPtr<Arcball>          camera;
    SmartPtr<Time>             time;
    SmartPtr<GLTFAssetManager> manager;

    SmartPtr<scene::Builder>   builder;
    SmartPtr<scene::Node>      root;
    SmartPtr<GPUImage>         dfglut;
    SmartPtr<Sampler>          diffuseSampler;
    SmartPtr<Sampler>          specularSampler;
    SmartPtr<Sampler>          dfglutSampler;

    GLTFModel model;
};

#endif // GLTFVIEWER_VIEWER_H
