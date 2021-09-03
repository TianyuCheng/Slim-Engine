#include <slim/slim.hpp>

using namespace slim;

struct Material {
    alignas(16) glm::vec3 baseColor;
    alignas(16) glm::vec3 emitColor;
    alignas(4)  float metalness;
    alignas(4)  float roughness;
};

struct CameraData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
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

    // create builder
    auto sceneBuilder = SlimPtr<scene::Builder>(device);
    sceneBuilder->EnableRayTracing();
    sceneBuilder->EnableASCompaction();

    // create meshes
    auto cubeMesh = sceneBuilder->CreateMesh();
    auto planeMesh = sceneBuilder->CreateMesh();
    {
        auto cubeData = Cube { 0.5f, 0.5f, 0.5f }.Create();
        cubeMesh->SetIndexBuffer(cubeData.indices);
        cubeMesh->SetVertexBuffer(cubeData.vertices);

        auto planeData = Plane {}.Create();
        planeMesh->SetIndexBuffer(planeData.indices);
        planeMesh->SetVertexBuffer(planeData.vertices);
    }

    // create scene
    auto sceneRoot = sceneBuilder->CreateNode("root");
    auto floorNode = sceneBuilder->CreateNode("floor", sceneRoot);
    auto ceilingNode = sceneBuilder->CreateNode("ceiling", sceneRoot);
    auto leftWallNode = sceneBuilder->CreateNode("leftWall", sceneRoot);
    auto rightWallNode = sceneBuilder->CreateNode("rightWall", sceneRoot);
    auto backWallNode = sceneBuilder->CreateNode("backWall", sceneRoot);
    auto shortBoxNode = sceneBuilder->CreateNode("shortBox", sceneRoot);
    auto tallBoxNode = sceneBuilder->CreateNode("tallBox", sceneRoot);
    {
        sceneRoot->Scale(2.0, 2.0, 2.0);

        floorNode->Translate(0.0, -0.5, 0.0);
        floorNode->SetDraw(planeMesh, nullptr);

        ceilingNode->Translate(0.0, 0.5, 0.0);
        ceilingNode->Rotate(glm::vec3(1.0, 0.0, 0.0), M_PI);
        ceilingNode->SetDraw(planeMesh, nullptr);

        leftWallNode->Translate(-0.5, 0.0, 0.0);
        leftWallNode->Rotate(glm::vec3(1.0, 0.0, 0.0), M_PI);
        leftWallNode->Rotate(glm::vec3(0.0, 0.0, 1.0), -M_PI / 2);
        leftWallNode->SetDraw(planeMesh, nullptr);

        rightWallNode->Translate(0.5, 0.0, 0.0);
        rightWallNode->Rotate(glm::vec3(1.0, 0.0, 0.0), M_PI);
        rightWallNode->Rotate(glm::vec3(0.0, 0.0, 1.0), M_PI / 2);
        rightWallNode->SetDraw(planeMesh, nullptr);

        backWallNode->Translate(0.0, 0.0, -0.5);
        backWallNode->Rotate(glm::vec3(1.0, 0.0, 0.0), M_PI / 2.0);
        backWallNode->SetDraw(planeMesh, nullptr);

        shortBoxNode->Translate(0.2, -0.25, 0.2);
        shortBoxNode->Rotate(glm::vec3(0.0, 1.0, 0.0), M_PI / 4.0);
        shortBoxNode->Scale(0.5, 0.5, 0.5);
        shortBoxNode->SetDraw(cubeMesh, nullptr);

        tallBoxNode->Translate(-0.2, -0.15, -0.2);
        tallBoxNode->Rotate(glm::vec3(0.0, 1.0, 0.0), M_PI / 4.0);
        tallBoxNode->Scale(0.5, 1.0, 0.5);
        tallBoxNode->SetDraw(cubeMesh, nullptr);
    }
    sceneRoot->ApplyTransform();
    sceneBuilder->Build();

    // shaders
    auto rayGenShader = SlimPtr<spirv::RayGenShader>(device, "main", "shaders/raytrace.rgen.spv");
    auto rayMissShader = SlimPtr<spirv::MissShader>(device, "main", "shaders/raytrace.rmiss.spv");
    auto closestShader = SlimPtr<spirv::ClosestHitShader>(device, "main", "shaders/raytrace.rchit.spv");
    auto postVertexShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/post.vert.spv");
    auto postFragmentShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/post.frag.spv");

    // rt pipeline
    auto rtPipeline = SlimPtr<Pipeline>(
        device,
        RayTracingPipelineDesc()
            .SetName("simple-raytracing")
            .SetRayGenShader(rayGenShader)
            .SetMissShader(rayMissShader)
            .SetClosestHitShader(closestShader)
            .SetMaxRayRecursionDepth(2)
            .SetPipelineLayout(PipelineLayoutDesc()
                .AddBinding("Accel",  0, 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                .AddBinding("Image",  0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                .AddBinding("Camera", 1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            ));

    // persistent resources
    auto sampler = SlimPtr<Sampler>(device, SamplerDesc {});
    auto rtImage = SlimPtr<GPUImage>(device,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VkExtent2D { 512, 512 }, 1, 1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    auto cameraUniformBuffer = SlimPtr<Buffer>(device, sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    while (window->IsRunning()) {
        Window::PollEvents();

        // get current render frame
        auto frame = window->AcquireNext();

        uint32_t width = frame->GetExtent().width;
        uint32_t height = frame->GetExtent().height;

        // update camera perspective matrix based on frame aspect ratio
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0);

        CameraData cameraData;
        cameraData.view = camera->GetView();
        cameraData.proj = camera->GetProjection();
        cameraData.viewInverse = glm::inverse(cameraData.view);
        cameraData.projInverse = glm::inverse(cameraData.proj);

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto rtBuffer = renderGraph.CreateResource(rtImage);

            auto rtPass = renderGraph.CreateComputePass("raytrace");
            rtPass->SetStorage(rtBuffer);
            rtPass->Execute([&](const RenderInfo &info) {
                // calling ray tracing function here
                CommandBuffer* commandBuffer = info.commandBuffer;
                RenderFrame* renderFrame = info.renderFrame;

                commandBuffer->CopyDataToBuffer(cameraData, cameraUniformBuffer);

                // bind pipeline
                commandBuffer->BindPipeline(rtPipeline);

                // bind descriptor
                auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), rtPipeline->Layout());
                descriptor->SetAccelStruct("Accel", sceneBuilder->GetAccelBuilder()->GetTlas());
                descriptor->SetStorageImage("Image", rtBuffer->GetImage());
                descriptor->SetUniform("Camera", cameraUniformBuffer);
                commandBuffer->BindDescriptor(descriptor, rtPipeline->Type());

                // ray trace
                vkCmdTraceRaysKHR(*commandBuffer,
                                  rtPipeline->GetRayGenRegion(),
                                  rtPipeline->GetMissRegion(),
                                  rtPipeline->GetHitRegion(),
                                  rtPipeline->GetCallableRegion(),
                                  width, height, 1);
            });

            auto colorPass = renderGraph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetTexture(rtBuffer);
            colorPass->Execute([&](const RenderInfo &info) {
                CommandBuffer* commandBuffer = info.commandBuffer;
                RenderFrame* renderFrame = info.renderFrame;

                auto pipeline = renderFrame->RequestPipeline(
                    GraphicsPipelineDesc()
                        .SetName("post-raytracing")
                        .SetVertexShader(postVertexShader)
                        .SetFragmentShader(postFragmentShader)
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_BACK_BIT)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(info.renderPass)
                        .SetDepthTest(VK_COMPARE_OP_LESS)
                        .SetPipelineLayout(PipelineLayoutDesc()
                            .AddBinding("MainTex", 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)));

                commandBuffer->BindPipeline(pipeline);

                // bind descriptor
                auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), pipeline->Layout());
                descriptor->SetTexture("MainTex", rtBuffer->GetImage(), sampler);
                commandBuffer->BindDescriptor(descriptor, pipeline->Type());

                // draw
                commandBuffer->Draw(6, 1, 0, 0);
            });
        }
        renderGraph.Execute();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
