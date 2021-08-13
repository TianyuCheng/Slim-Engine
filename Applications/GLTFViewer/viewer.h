#ifndef GLTFVIEWER_VIEWER_H
#define GLTFVIEWER_VIEWER_H

#include <slim/slim.hpp>
#include "config.h"
#include "model.h"
#include "gizmo.h"

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
    void LoadModel();

private:
    SmartPtr<Context>          context;
    SmartPtr<Device>           device;
    SmartPtr<Window>           window;
    SmartPtr<Input>            input;
    SmartPtr<Gizmo>            gizmo;
    SmartPtr<Camera>           camera;
    SmartPtr<GLTFAssetManager> manager;

    GLTFModel model;
    Arcball arcball;
};

#endif // GLTFVIEWER_VIEWER_H
