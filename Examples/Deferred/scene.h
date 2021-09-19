#include <slim/slim.hpp>

using namespace slim;

class MainScene {
public:
    MainScene(Device* device) {
        // create vertex and fragment shaders
        auto vShader = SlimPtr<spirv::VertexShader>(device, "main", "shaders/gbuffer.vert.spv");
        auto fShader = SlimPtr<spirv::FragmentShader>(device, "main", "shaders/gbuffer.frag.spv");

        // create blend states
        VkPipelineColorBlendAttachmentState blend = {};
        {
            blend.blendEnable = VK_FALSE;
            blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                 | VK_COLOR_COMPONENT_G_BIT
                                 | VK_COLOR_COMPONENT_B_BIT
                                 | VK_COLOR_COMPONENT_A_BIT;
        }

        // create technique
        technique = SlimPtr<Technique>();
        technique->AddPass(RenderQueue::Opaque,
            GraphicsPipelineDesc()
                .SetName("textured")
                .AddVertexBinding(0, sizeof(gltf::Vertex), VK_VERTEX_INPUT_RATE_VERTEX, {
                    { 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, position)) },
                    { 1, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(gltf::Vertex, normal  )) },
                    { 2, VK_FORMAT_R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(gltf::Vertex, uv0     )) },
                 })
                .SetVertexShader(vShader)
                .SetFragmentShader(fShader)
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetDepthTest(VK_COMPARE_OP_LESS)
                .SetBlendState(0, blend)
                .SetBlendState(1, blend)
                .SetBlendState(2, blend)
                .SetBlendState(3, blend)
                .SetPipelineLayout(PipelineLayoutDesc()
                    .AddBinding("Camera",    SetBinding { 0, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("Model",     SetBinding { 1, 0 }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding("MainTex",   SetBinding { 2, 0 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                ));

        // model loading
        builder = SlimPtr<scene::Builder>(device);
        model.Load(builder, ToAssetPath("Scenes/Sponza/glTF/Sponza.gltf"));

        // update materials
        for (auto& material : model.materials) {
            material->SetTechnique(technique);

            // we only need base color texture
            const gltf::MaterialData& data = material->GetData<gltf::MaterialData>();
            material->SetTexture("MainTex", model.images[data.baseColorTexture], model.samplers[data.baseColorSampler]);
        }

        // create scene
        builder->Build();
    }

    void DrawGBuffer(const RenderInfo& info, Camera* camera) {
        // sceneFilter result + sorting
        auto culling = CPUCulling();
        culling.Cull(model.GetScene(0), camera);
        culling.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        culling.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);
        // rendering
        MeshRenderer renderer(info);
        renderer.Draw(camera, culling.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
    }

    SmartPtr<Device>                device;
    SmartPtr<spirv::VertexShader>   vShader;
    SmartPtr<spirv::FragmentShader> fShader;
    SmartPtr<Technique>             technique;
    SmartPtr<scene::Builder>        builder;
    gltf::Model                     model;
};
