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
    constexpr static int X = 8;
    constexpr static int Z = 8;

public:
    Fairies(Device* device) : device(device) {
        PrepareMaskPipeline();
        PrepareLightPipeline();
        PrepareFairyPipeline();
        PrepareGeometry();
        PrepareInstances();
    }

    void PrepareMaskPipeline() {
        vShaderMask = SlimPtr<spirv::VertexShader>(device, "main", "shaders/fairymask.vert.spv");
        fShaderMask = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/fairymask.frag.spv");

        // blend
        VkPipelineColorBlendAttachmentState blend = {};
        blend.blendEnable = VK_FALSE;
        blend.colorWriteMask = 0; // no color writes

        // stencil
        VkStencilOpState stencil = {};
        stencil.passOp = VK_STENCIL_OP_REPLACE;
        stencil.failOp = VK_STENCIL_OP_KEEP;
        stencil.depthFailOp = VK_STENCIL_OP_KEEP;
        stencil.compareOp = VK_COMPARE_OP_ALWAYS;
        stencil.reference = 0x1;
        stencil.compareMask = 0xff;
        stencil.writeMask = 0xff;

        // mask pipeline
        maskPipelineDesc =
            GraphicsPipelineDesc()
                .SetName("mask")
                .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                    { 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(GeometryData::Vertex, position)) },
                 })
                .AddVertexBinding(1, sizeof(FairyInfo), VK_VERTEX_INPUT_RATE_INSTANCE, {
                    { 1, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(FairyInfo, position)) },
                    { 2, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(FairyInfo, color)   ) },
                    { 3, VK_FORMAT_R32_SFLOAT,       static_cast<uint32_t>(offsetof(FairyInfo, radius)  ) },
                 })
                .SetVertexShader(vShaderMask)
                .SetFragmentShader(fShaderMask)
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetDepthTest(VK_COMPARE_OP_LESS, false)
                .SetStencilTest(stencil, stencil)
                // .SetBlendState(0, blend)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,   VK_SHADER_STAGE_VERTEX_BIT)
                );
    }

    void PrepareLightPipeline() {
        vShaderLight = SlimPtr<spirv::VertexShader>(device, "main", "shaders/fairylight.vert.spv");
        fShaderLight = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/fairylight.frag.spv");

        // blend
        VkPipelineColorBlendAttachmentState blend = {};
        blend.blendEnable = VK_TRUE;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                             | VK_COLOR_COMPONENT_G_BIT
                             | VK_COLOR_COMPONENT_B_BIT
                             | VK_COLOR_COMPONENT_A_BIT;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

        // stencil
        VkStencilOpState stencil = {};
        stencil.passOp = VK_STENCIL_OP_KEEP;
        stencil.failOp = VK_STENCIL_OP_KEEP;
        stencil.depthFailOp = VK_STENCIL_OP_KEEP;
        stencil.compareOp = VK_COMPARE_OP_EQUAL;
        stencil.reference = 0x1;
        stencil.compareMask = 0xff;
        stencil.writeMask = 0xff;

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
                .SetBlendState(0, blend)
                .SetStencilTest(stencil, stencil)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,   VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("Albedo",    SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Normal",    SetBinding { 1, 1 }, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding("Position",  SetBinding { 1, 2 }, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
                );
    }

    void PrepareFairyPipeline() {
        vShaderFairy = SlimPtr<spirv::VertexShader>(device, "main", "shaders/fairy.vert.spv");
        fShaderFairy = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/fairy.frag.spv");

        // stencil
        VkStencilOpState stencil = {};
        stencil.passOp = VK_STENCIL_OP_KEEP;
        stencil.failOp = VK_STENCIL_OP_KEEP;
        stencil.depthFailOp = VK_STENCIL_OP_KEEP;
        stencil.compareOp = VK_COMPARE_OP_EQUAL;
        stencil.reference = 0x1;
        stencil.compareMask = 0xff;
        stencil.writeMask = 0xff;

        // mask pipeline
        fairyPipelineDesc =
            GraphicsPipelineDesc()
                .SetName("fairy")
                .AddVertexBinding(0, sizeof(GeometryData::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                    { 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(GeometryData::Vertex, position)) },
                 })
                .AddVertexBinding(1, sizeof(FairyInfo), VK_VERTEX_INPUT_RATE_INSTANCE, {
                    { 1, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(FairyInfo, position)) },
                    { 2, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(FairyInfo, color)   ) },
                 })
                .SetVertexShader(vShaderFairy)
                .SetFragmentShader(fShaderFairy)
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetDepthTest(VK_COMPARE_OP_LESS, true)
                // .SetStencilTest(stencil, stencil)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,   VK_SHADER_STAGE_VERTEX_BIT)
                );
    }

    void PrepareGeometry() {
        // sphere geometry
        auto sphereData = Sphere { 1.0f, 8, 8 }.Create();
        sphereBuilder = SlimPtr<scene::Builder>(device);
        sphereGeometry = sphereBuilder->CreateMesh();
        sphereGeometry->SetIndexBuffer(sphereData.indices);
        sphereGeometry->SetVertexBuffer(sphereData.vertices);
        sphereBuilder->Build();
        indexCount = sphereData.indices.size();
    }

    void PrepareInstances() {
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
        for (int x = -X; x < X; x++) {
            for (int z = -Z; z < Z; z++) {
                infos.push_back(FairyInfo {
                    glm::vec3(static_cast<float>(x) * 55.0f, 0.0f,
                              static_cast<float>(z) * 55.0f),
                    colors[(x * 16 + z) % colors.size()],
                    100.0f
                });
                heights.push_back(100.0f + 100.0f * dis(gen));
            }
        }
        instanceCount = infos.size();
        instanceBuffer = SlimPtr<Buffer>(device, infos.size() * sizeof(FairyInfo),
                                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         VMA_MEMORY_USAGE_GPU_ONLY);
    }

    void UpdateInstances(Time* time) {
        for (uint32_t i = 0; i < instanceCount; i++) {
            infos[i].position.y = heights[i] + std::sin(time->Elapsed() + infos[i].position.x + infos[i].position.z) * 50.0f;
        }
        device->Execute([&](CommandBuffer* commandBuffer) {
            commandBuffer->CopyDataToBuffer(infos, instanceBuffer);
        });
    }

    void DrawMask(const RenderInfo& info, Camera* camera) {
        // bind pipeline
        auto pipeline = info.renderFrame->RequestPipeline(
            maskPipelineDesc
                .SetRenderPass(info.renderPass)
                .SetViewport(info.renderFrame->GetExtent())
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
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);

        // bind vertex and index
        sphereGeometry->Bind(info.commandBuffer);

        // bind instance
        info.commandBuffer->BindVertexBuffer(1, instanceBuffer, 0);

        // draw
        info.commandBuffer->DrawIndexed(indexCount, instanceCount, 0, 0, 0);
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

    void DrawFairy(const RenderInfo& info, Camera* camera) {
        // bind pipeline
        auto pipeline = info.renderFrame->RequestPipeline(
            fairyPipelineDesc
                .SetRenderPass(info.renderPass)
                .SetViewport(info.renderFrame->GetExtent())
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
        info.commandBuffer->BindDescriptor(descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);

        // bind vertex and index
        sphereGeometry->Bind(info.commandBuffer);

        // bind instance
        info.commandBuffer->BindVertexBuffer(1, instanceBuffer, 0);

        // draw
        info.commandBuffer->DrawIndexed(indexCount, instanceCount, 0, 0, 0);
    }

    SmartPtr<Device>                device;

    GraphicsPipelineDesc            maskPipelineDesc;
    SmartPtr<spirv::VertexShader>   vShaderMask;
    SmartPtr<spirv::FragmentShader> fShaderMask;

    GraphicsPipelineDesc            lightPipelineDesc;
    SmartPtr<spirv::VertexShader>   vShaderLight;
    SmartPtr<spirv::FragmentShader> fShaderLight;

    GraphicsPipelineDesc            fairyPipelineDesc;
    SmartPtr<spirv::VertexShader>   vShaderFairy;
    SmartPtr<spirv::FragmentShader> fShaderFairy;

    SmartPtr<scene::Builder>        sphereBuilder;
    SmartPtr<scene::Mesh>           sphereGeometry;

    SmartPtr<Buffer>                instanceBuffer;
    uint32_t                        indexCount;
    uint32_t                        instanceCount;
    std::vector<FairyInfo>          infos;
    std::vector<float>              heights;
};
