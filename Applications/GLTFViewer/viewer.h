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
    SmartPtr<Arcball>          arcball;
    SmartPtr<GLTFAssetManager> manager;

    SmartPtr<SceneManager>     scene;
    SmartPtr<Scene>            root;
    SmartPtr<GPUImage>         dfglut;
    SmartPtr<Sampler>          diffuseSampler;
    SmartPtr<Sampler>          specularSampler;
    SmartPtr<Sampler>          dfglutSampler;

    GLTFModel model;
};

#endif // GLTFVIEWER_VIEWER_H
