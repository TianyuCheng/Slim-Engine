#include <slim/slim.hpp>

using namespace slim;

struct RefractProperties {
    float iota;
};

int main() {
    slim::Initialize();

    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .Verbose(true)
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
    );

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(640, 640)
            .SetResizable(true)
            .SetTitle("OctMap")
    );

    auto decodeVShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/decode_hemioct.vert.spv");
    auto decodeFShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/decode_hemioct.frag.spv");

    // SmartPtr<GPUImage> image = nullptr;
    SmartPtr<GPUImage> cubemap = nullptr;
    SmartPtr<Sampler> sampler = nullptr;
    device->Execute([&](CommandBuffer* commandBuffer) {
        // image = TextureLoader::Load2D(commandBuffer,
        //         GetUserAsset("Skyboxes/NiagaraFalls/posx.jpg"));
        cubemap = TextureLoader::LoadCubemap(commandBuffer,
                GetUserAsset("Skyboxes/NiagaraFalls/posx.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negx.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/posy.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negy.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/posz.jpg"),
                GetUserAsset("Skyboxes/NiagaraFalls/negz.jpg"));
        sampler = SlimPtr<Sampler>(device, SamplerDesc());
    });

    // render
    while (window->IsRunning()) {
        Window::PollEvents();

        // query image from swapchain
        auto frame = window->AcquireNext();

        // rendergraph-based design
        RenderGraph renderGraph(frame);
        {
            auto colorBuffer = renderGraph.CreateResource(frame->GetBackBuffer());
            auto decodePass = renderGraph.CreateRenderPass("decode");
            decodePass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            decodePass->Execute([&](const RenderInfo &info) {
                auto pipeline = frame->RequestPipeline(
                    GraphicsPipelineDesc()
                        .SetName("decodePass")
                        .SetVertexShader(decodeVShader)
                        .SetFragmentShader(decodeFShader)
                        .SetViewport(frame->GetExtent())
                        .SetCullMode(VK_CULL_MODE_BACK_BIT)
                        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                        .SetRenderPass(info.renderPass)
                        .SetPipelineLayout(PipelineLayoutDesc()
                            .AddBinding("Skybox", SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        )
                    );

                // bind pipeline
                info.commandBuffer->BindPipeline(pipeline);

                // bind descriptor
                auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
                descriptor->SetTexture("Skybox", cubemap, sampler);
                info.commandBuffer->BindDescriptor(descriptor, pipeline->Type());

                // draw quad
                info.commandBuffer->Draw(6, 1, 0, 0);
            });
        }
        renderGraph.Execute();
    }

    device->WaitIdle();
    return EXIT_SUCCESS;
}
