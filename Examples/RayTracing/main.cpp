#include <slim/slim.hpp>

using namespace slim;

struct Material {
    alignas(16) glm::vec3 baseColor;
    alignas(16) glm::vec3 emitColor;
    alignas(4)  float metalness;
    alignas(4)  float roughness;
};

int main() {
    slim::Initialize();

    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableRayTracing()
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(512, 512)
            .SetResizable(true)
            .SetTitle("Ray Tracing")
    );

    // create a scene camera
    auto camera = SlimPtr<Camera>("camera");
    camera->Perspective(1.05, 1.0, 0.1, 20.0);
    camera->LookAt(glm::vec3(0.0, 0.0, 3.0),
                   glm::vec3(0.0, 0.0, 0.0),
                   glm::vec3(0.0, 1.0, 0.0));

    // create builders
    auto sceneBuilder = SlimPtr<scene::Builder>(device);
    auto accelBuilder = SlimPtr<accel::Builder>(device);
    sceneBuilder->EnableRayTracing();
    accelBuilder->EnableCompaction();

    // create mesh
    auto cubeMesh = sceneBuilder->CreateMesh();
    {
        auto cubeData = Cube{}.Create();
        cubeMesh->SetVertexBuffer<GeometryData::Vertex>(cubeData.vertices);
        cubeMesh->SetIndexBuffer<uint32_t>(cubeData.indices);
        cubeMesh->AddInputBinding(0, 0);
    }

    // create scene
    auto sceneRoot = sceneBuilder->CreateNode("root");
    auto sphereNode = sceneBuilder->CreateNode("cube", sceneRoot);
    {
        sphereNode->SetDraw(cubeMesh, nullptr);
    }
    sceneRoot->ApplyTransform();
    device->Execute([&](CommandBuffer* commandBuffer) {
        sceneBuilder->Build(commandBuffer);
    });

    // create acceleration structure
    for (auto& mesh : sceneBuilder->meshes) {
        accelBuilder->AddMesh(mesh, sizeof(GeometryData));
    }
    accelBuilder->BuildBlas();

    for (auto& node : sceneBuilder->nodes) {
        accelBuilder->AddNode(node);
    }
    accelBuilder->BuildTlas();

    // // while (window->IsRunning()) {
    // //     Window::PollEvents();
    // //
    // //     // get current render frame
    // //     auto frame = window->AcquireNext();
    // //
    // //     // update camera perspective matrix based on frame aspect ratio
    // //     camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0);
    // // }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
