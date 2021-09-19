#include <random>
#include <slim/slim.hpp>

using namespace slim;

struct FairyInfo {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
};

struct CameraInfo {
    glm::mat4 V;
    glm::mat4 P;
};

class Fairies {
public:
    Fairies(Device* device) : device(device) {
        vShaderMask = SlimPtr<spirv::VertexShader>(device, "main", "shaders/fairymask.vert.spv");
        fShaderMask = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/fairymask.frag.spv");
        vShaderLight = SlimPtr<spirv::VertexShader>(device, "main", "shaders/fairylight.vert.spv");
        fShaderLight = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/fairylight.frag.spv");

        // blend
        VkPipelineColorBlendAttachmentState lightBlendState = {};
        lightBlendState.blendEnable = VK_TRUE;
        lightBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                       | VK_COLOR_COMPONENT_G_BIT
                                       | VK_COLOR_COMPONENT_B_BIT
                                       | VK_COLOR_COMPONENT_A_BIT;
        lightBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
        lightBlendState.colorBlendOp = VK_BLEND_OP_ADD;
        lightBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        lightBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        lightBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        lightBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

        // light pipeline
        lightPipelineDesc =
            GraphicsPipelineDesc()
                .SetName("light")
                .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                    { 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(GeometryData::Vertex, position)) },
                 })
                .AddVertexBinding(1, sizeof(FairyInfo), VK_VERTEX_INPUT_RATE_INSTANCE, {
                    { 1, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(FairyInfo, position)) },
                    { 2, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(FairyInfo, color)   ) },
                    { 3, VK_FORMAT_R32_SFLOAT,       static_cast<uint32_t>(offsetof(FairyInfo, radius)  ) },
                 })
                .SetVertexShader(vShaderLight)
                .SetFragmentShader(fShaderLight)
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetDepthTest(VK_COMPARE_OP_LESS, false)
                .SetBlendState(0, lightBlendState)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,   VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("Albedo",    SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Normal",    SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Position",  SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
                );

        // sphere geometry
        auto sphereData = Sphere { 1.0f, 8, 8 }.Create();
        sphereBuilder = SlimPtr<scene::Builder>(device);
        sphereGeometry = sphereBuilder->CreateMesh();
        sphereGeometry->SetIndexBuffer(sphereData.indices);
        sphereGeometry->SetVertexBuffer(sphereData.vertices);
        sphereBuilder->Build();
        indexCount = sphereData.indices.size();

        std::default_random_engine gen;
        std::uniform_real_distribution<double> dis(-1.0, 1.0);

        // instance buffer (vertex attribute)
        std::vector<glm::vec3> colors = {
            glm::vec3(1.0, 0.0, 0.0),
            glm::vec3(0.0, 1.0, 0.0),
            glm::vec3(0.0, 0.0, 1.0),
            glm::vec3(1.0, 1.0, 0.0),
            glm::vec3(0.0, 1.0, 1.0),
            glm::vec3(1.0, 0.0, 1.0),
        };
        std::vector<FairyInfo> infos;
        constexpr int X = 16;
        constexpr int Z = 8;
        for (int x = -X; x < X; x++) {
            for (int z = -Z; z < Z; z++) {
                infos.push_back(FairyInfo {
                    glm::vec3(static_cast<float>(x) * 55.0f,
                              150.0f + dis(gen) * 100.0f,
                              static_cast<float>(z) * 55.0f),
                    colors[(x * 16 + z) % colors.size()],
                    30.0f
                });
            }
        }
        instanceCount = infos.size();
        instanceBuffer = SlimPtr<Buffer>(device, infos.size() * sizeof(FairyInfo),
                                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         VMA_MEMORY_USAGE_GPU_ONLY);
        device->Execute([&](CommandBuffer* commandBuffer) {
            commandBuffer->CopyDataToBuffer(infos, instanceBuffer);
        });
    }

    void DrawLight(const RenderInfo& info, Camera* camera, Image* albedo, Image* normal, Image* position) {
        uint32_t subpass = 1;

        // bind pipeline
        auto pipeline = info.renderFrame->RequestPipeline(
            lightPipelineDesc
                .SetRenderPass(info.renderPass)
                .SetViewport(info.renderFrame->GetExtent()),
            subpass
        );
        info.commandBuffer->BindPipeline(pipeline);

        // uniform data
        CameraInfo cameraInfo = {
            camera->GetView(),
            camera->GetProjection(),
        };
        auto cameraUniform = info.renderFrame->RequestUniformBuffer(cameraInfo);

        // bind descriptor
        auto descriptor = SlimPtr<Descriptor>(info.renderFrame->GetDescriptorPool(), pipeline->Layout());
        descriptor->SetUniformBuffer("Camera", cameraUniform);
        descriptor->SetInputAttachment("Albedo", albedo);
        descriptor->SetInputAttachment("Normal", normal);
        descriptor->SetInputAttachment("Position", position);
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);

        // bind vertex and index
        sphereGeometry->Bind(info.commandBuffer);

        // bind instance
        info.commandBuffer->BindVertexBuffer(1, instanceBuffer, 0);

        // draw
        info.commandBuffer->DrawIndexed(indexCount, instanceCount, 0, 0, 0);
    }

    SmartPtr<Device>                device;
    SmartPtr<spirv::VertexShader>   vShaderMask;
    SmartPtr<spirv::FragmentShader> fShaderMask;
    SmartPtr<spirv::VertexShader>   vShaderLight;
    SmartPtr<spirv::FragmentShader> fShaderLight;
    SmartPtr<scene::Builder>        sphereBuilder;
    SmartPtr<scene::Mesh>           sphereGeometry;
    SmartPtr<Buffer>                instanceBuffer;
    GraphicsPipelineDesc            maskPipelineDesc;
    GraphicsPipelineDesc            lightPipelineDesc;
    uint32_t                        indexCount;
    uint32_t                        instanceCount;
};
