#include <slim/slim.hpp>

using namespace slim;

enum SamplingType : uint32_t {
    UNIFORM = 0,
    COSINE  = 1,
    HEMIOCT = 2,
};

struct GenInfo {
    uint32_t seed;
    uint32_t count;
    SamplingType type;
};

void Generate(Device* device, Pipeline* pipeline, Buffer* in, Buffer* out, const GenInfo& info) {
    auto pool = SlimPtr<DescriptorPool>(device, 1);
    auto descriptor = SlimPtr<Descriptor>(pool, pipeline->Layout());
    descriptor->SetStorageBuffer("Input", in);
    descriptor->SetStorageBuffer("Output", out);

    device->Execute([&](CommandBuffer* commandBuffer) {
        int groupX = (info.count + 31) / 32;
        commandBuffer->BindPipeline(pipeline);
        commandBuffer->BindDescriptor(descriptor, pipeline->Type());
        commandBuffer->PushConstants(pipeline->Layout(), "Info", &info);
        commandBuffer->Dispatch(groupX, 1, 1);
        commandBuffer->PrepareForBuffer(in, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        commandBuffer->PrepareForBuffer(out, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    });
}

int main() {
    slim::Initialize();

    // create a device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .Verbose(true)
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableNonSolidPolygonMode()
    );

    // create a device
    auto device = SlimPtr<Device>(context);

    // create a window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 640)
            .SetResizable(true)
            .SetTitle("Hemisphere")
    );

    // ui and input
    auto input = SlimPtr<Input>(window);
    auto ui = SlimPtr<DearImGui>(device, window);

    // create a camera
    auto camera = SlimPtr<Arcball>();
    camera->LookAt(glm::vec3(-2.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    // create shaders
    auto cShader = SlimPtr<spirv::ComputeShader>(device, "main", "shaders/generate.comp.spv");
    auto vShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/direction.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/direction.frag.spv");

    auto cPipeline = SlimPtr<Pipeline>(
        device,
        ComputePipelineDesc()
            .SetName("Generate")
            .SetComputeShader(cShader)
            .SetPipelineLayout(
                PipelineLayoutDesc()
                    .AddPushConstant("Info", Range { 0, sizeof(GenInfo) }, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Input",  SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                    .AddBinding("Output", SetBinding { 0, 1 }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            )
    );

    constexpr int MAX_DIR_COUNT = 10000;

    // create buffers
    auto inBuffer = SlimPtr<Buffer>(
        device,
        sizeof(glm::vec2) * MAX_DIR_COUNT,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    inBuffer->SetName("inputUV");

    auto outBuffer = SlimPtr<Buffer>(
        device,
        sizeof(glm::vec4) * 2 * MAX_DIR_COUNT,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    outBuffer->SetName("outputDir");

    // generate random samples on hemisphere
    GenInfo gen = {};
    gen.seed = 0x1234;
    gen.count = 500;
    gen.type = UNIFORM;
    Generate(device, cPipeline, inBuffer, outBuffer, gen);

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // update camera perspective
        camera->SetExtent(window->GetExtent());
        camera->Update(input);
        camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 10.0);
        glm::mat4 mvp = camera->GetProjection() * camera->GetView();

        ui->Begin();
        {
            bool update = false;

            int *genType = (int*)(&gen.type);
            if (ImGui::RadioButton("Uniform Hemisphere Sampling", genType, UNIFORM)) {
                update = true;
            }
            if (ImGui::RadioButton("Cosine Weighted Sampling", genType, COSINE)) {
                update = true;
            }
            if (ImGui::RadioButton("Hemi-Octahedron Decoding", genType, HEMIOCT)) {
                update = true;
            }

            ImGui::Separator();

            if (ImGui::SliderInt("Count", (int32_t*) &gen.count, 1, MAX_DIR_COUNT)) {
                update = true;
            }

            if (update) {
                device->WaitIdle();
                Generate(device, cPipeline, inBuffer, outBuffer, gen);
            }
        }
        ui->End();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());

            auto decodePass = renderGraph.CreateRenderPass("render");
            decodePass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            decodePass->Execute([&](const RenderInfo &info) {
                auto pipeline = frame->RequestPipeline(
                    GraphicsPipelineDesc()
                        .SetName("colorPass")
                        .SetVertexShader(vShader)
                        .SetFragmentShader(fShader)
                        .AddVertexBinding(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX, {
                            VertexAttrib { 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 }
                        })
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_NONE)
                        .SetPrimitive(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(info.renderPass)
                        .SetPipelineLayout(
                            PipelineLayoutDesc()
                                .AddPushConstant("Camera", Range { 0, sizeof(glm::mat4) }, VK_SHADER_STAGE_VERTEX_BIT)
                        )
                    );

                // bind pipeline
                info.commandBuffer->BindPipeline(pipeline);

                // draw
                info.commandBuffer->PushConstants(pipeline->Layout(), "Camera", &mvp);
                info.commandBuffer->BindVertexBuffer(0, outBuffer, 0);
                info.commandBuffer->Draw(gen.count * 2, 1, 0, 0);
            });

            auto uiPass = renderGraph.CreateRenderPass("ui");
            uiPass->SetColor(colorBuffer);
            uiPass->Execute([&](const RenderInfo &info) {
                ui->Draw(info.commandBuffer);
            });
        }
        renderGraph.Execute();
        input->Reset();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
