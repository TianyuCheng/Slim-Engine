#include <slim/slim.hpp>
#include "Time.h"

using namespace slim;

int main() {
    // create a vulkan context
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true),
        WindowDesc()
            .SetResolution(640, 480)
            .SetResizable(true)
            .SetTitle("Slim Application")
    );

    // create texture
    auto renderFrame = SlimPtr<RenderFrame>(context);
    auto commandBuffer = renderFrame->RequestCommandBuffer(VK_QUEUE_TRANSFER_BIT);
    commandBuffer->Begin();
    SmartPtr<Sampler> sampler = SlimPtr<Sampler>(context, SamplerDesc());
    SmartPtr<GPUImage2D> texture = TextureLoader::Load2D(commandBuffer, ToAssetPath("Pictures/VulkanOpaque.png"), VK_FILTER_LINEAR);
    commandBuffer->End();
    commandBuffer->Submit();
    context->WaitIdle();

    // create vertex and fragment shaders
    auto vShader = SlimPtr<spirv::VertexShader>(context, "main", "shaders/textured_vertex.vert.spv");
    auto fShader = SlimPtr<spirv::FragmentShader>(context, "main", "shaders/textured_fragment.frag.spv");

    // create a material, attach a graphics pipeline
    auto material = SlimPtr<Material>("material", Material::Opaque);
    material->GetPipelineDesc()
        .SetName("textured")
        .AddVertexBinding(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX, {
            { 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }
         })
        .AddVertexBinding(1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX, {
            { 1, VK_FORMAT_R32G32_SFLOAT, 0 }
         })
        .SetVertexShader(vShader)
        .SetFragmentShader(fShader)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetDepthTest(VK_COMPARE_OP_LESS)
        .SetPipelineLayout(PipelineLayoutDesc()
            .AddBinding("Camera", 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .AddBinding("Albedo", 1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));
    material->PrepareMaterial = [=](const RenderInfo &info) {
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame, material->GetPipeline());
        descriptor->SetTexture("Albedo", texture, sampler);
        info.commandBuffer->BindDescriptor(descriptor);
    };
    material->PrepareSceneNode = [=](const RenderInfo &info) {
        auto mvp = info.camera->GetViewProjection() * info.sceneNode->GetTransform();
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame, info.material->GetPipeline());
        descriptor->SetUniform("Camera", info.renderFrame->RequestUniformBuffer(mvp));
        info.commandBuffer->BindDescriptor(descriptor);
    };

    // create a mesh
    auto mesh = SlimPtr<Mesh>();
    context->Execute([=](CommandBuffer *commandBuffer) {
        // prepare vertex data
        std::vector<glm::vec3> positions = {
            // position
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f),

            // position
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),
        };

        std::vector<glm::vec2> texcoords = {
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),

            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),
        };

        // prepare index data
        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,
            4, 7, 6, 6, 5, 4,
            3, 7, 4, 4, 0, 3,
            1, 5, 6, 6, 2, 1,
            6, 7, 3, 3, 2, 6,
            4, 5, 1, 1, 0, 4
        };

        mesh->SetVertexAttrib(commandBuffer, positions, 0);
        mesh->SetVertexAttrib(commandBuffer, texcoords, 1);
        mesh->SetIndexAttrib(commandBuffer, indices);
    });

    // create a submesh
    auto submesh = Submesh(mesh, 0, 0, 36);

    // create a scene
    auto scene = SlimPtr<Scene>("scene");

    auto child1 = scene->AddChild("child1");
    child1->SetMesh(submesh);
    child1->SetMaterial(material);
    child1->Translate(1.0f, 0.0f, 0.0f);
    child1->Scale(1.5f, 1.5f, 1.5f);
    child1->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), M_PI / 4);

    auto child2 = scene->AddChild("child2");
    child2->SetMesh(submesh);
    child2->SetMaterial(material);
    child2->Translate(-1.0f, 0.0f, 0.0f);
    child2->Scale(0.5f, 0.5f, 0.5f);
    child2->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -M_PI / 4);

    scene->Init();

    // render
    auto window = context->GetWindow();
    while (!window->ShouldClose()) {
        // query image from swapchain
        auto frame = window->AcquireNext();

        // create a camera
        Camera camera("camera");
        camera.LookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        camera.Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0f);

        child1->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 0.025);
        child2->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 0.025);

        // update scene
        scene->Update();

        // rendergraph-based design
        RenderGraph graph(frame);
        {
            auto colorBuffer = graph.CreateResource(frame->GetBackBuffer());
            auto depthBuffer = graph.CreateResource(frame->GetExtent(), VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT);

            auto colorPass = graph.CreateRenderPass("color");
            colorPass->SetColor(colorBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            colorPass->SetDepthStencil(depthBuffer, ClearValue(1.0f, 0));
            colorPass->Execute([=](const RenderGraph &graph) {
                scene->Render(graph, camera);
            });
        }

        graph.Execute();

        // window update
        window->PollEvents();
    }
    context->WaitIdle();
    return EXIT_SUCCESS;
}
