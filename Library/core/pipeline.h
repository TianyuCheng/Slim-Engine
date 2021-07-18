#ifndef SLIM_CORE_PIPELINE_H
#define SLIM_CORE_PIPELINE_H

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "core/image.h"
#include "core/buffer.h"
#include "core/shader.h"
#include "core/sampler.h"
#include "core/context.h"
#include "core/renderpass.h"
#include "utility/interface.h"

namespace slim {

    class Pipeline;
    class Descriptor;
    class RenderFrame;

    //  ____  _            _ _            _                            _   ____
    // |  _ \(_)_ __   ___| (_)_ __   ___| |    __ _ _   _  ___  _   _| |_|  _ \  ___  ___  ___
    // | |_) | | '_ \ / _ \ | | '_ \ / _ \ |   / _` | | | |/ _ \| | | | __| | | |/ _ \/ __|/ __|
    // |  __/| | |_) |  __/ | | | | |  __/ |__| (_| | |_| | (_) | |_| | |_| |_| |  __/\__ \ (__
    // |_|   |_| .__/ \___|_|_|_| |_|\___|_____\__,_|\__, |\___/ \__,_|\__|____/ \___||___/\___|
    //         |_|                                   |___/

    struct DescriptorSetLayoutBinding {
        std::string                  name;
        VkDescriptorSetLayoutBinding binding;
    };

    class PipelineLayoutDesc final {
        friend class PipelineLayout;
        friend class std::hash<PipelineLayoutDesc>;
    public:
        PipelineLayoutDesc& AddBinding(const std::string &name, uint32_t set, uint32_t binding,
                                       VkDescriptorType descriptorType, VkShaderStageFlags stages);
        PipelineLayoutDesc& AddBindingArray(const std::string &name, uint32_t set, uint32_t binding, uint32_t count,
                                            VkDescriptorType descriptorType, VkShaderStageFlags stages);
    private:
        mutable std::multimap<uint32_t, std::vector<DescriptorSetLayoutBinding>> bindings;
    };

    //  ____  _            _ _            _                            _
    // |  _ \(_)_ __   ___| (_)_ __   ___| |    __ _ _   _  ___  _   _| |_
    // | |_) | | '_ \ / _ \ | | '_ \ / _ \ |   / _` | | | |/ _ \| | | | __|
    // |  __/| | |_) |  __/ | | | | |  __/ |__| (_| | |_| | (_) | |_| | |_
    // |_|   |_| .__/ \___|_|_|_| |_|\___|_____\__,_|\__, |\___/ \__,_|\__|
    //         |_|                                   |___/

    class PipelineLayout final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkPipelineLayout> {
        friend class Descriptor;
        friend class std::hash<PipelineLayout>;
    public:
        explicit PipelineLayout(Context *context, const PipelineLayoutDesc &desc);
        virtual ~PipelineLayout();
    private:
        void Init(std::multimap<uint32_t, std::vector<DescriptorSetLayoutBinding>> &bindings);
    private:
        SmartPtr<Context> context;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::unordered_map<std::string, std::pair<VkDescriptorSetLayout, uint32_t>> mappings;
        size_t hashValue;
    };

    //  ____                      _       _
    // |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __
    // | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__|
    // | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |
    // |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|
    //                             |_|

    class Descriptor final : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class CommandBuffer;
    public:
        explicit Descriptor(RenderFrame *frame, Pipeline *pipeline);
        virtual ~Descriptor();
        void SetUniform(const std::string &name, Buffer *buffer, size_t offset = 0, size_t size = 0);
        void SetStorage(const std::string &name, Buffer *buffer, size_t offset = 0, size_t size = 0);
        void SetTexture(const std::string &name, Image *texture, Sampler *sampler);
    private:
        void Update();
        std::pair<VkDescriptorSet, uint32_t> FindDescriptorSet(const std::string &name);
        void SetBuffer(const std::string &name, Buffer *buffer, VkDescriptorType descriptorType, size_t offset, size_t size);
    private:
        SmartPtr<RenderFrame> renderFrame;
        SmartPtr<Pipeline> pipeline;
        std::vector<VkWriteDescriptorSet> writes;
        std::list<VkDescriptorBufferInfo> bufferInfos;
        std::list<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    //  ____                      _       _             ____             _
    // |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __|  _ \ ___   ___ | |
    // | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| |_) / _ \ / _ \| |
    // | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |  |  __/ (_) | (_) | |
    // |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|  |_|   \___/ \___/|_|
    //                             |_|

    class DescriptorPool final {
    public:
        explicit DescriptorPool(Context *context, uint32_t poolSize);
        virtual ~DescriptorPool();
        void Reset();
        VkDescriptorSet Request(VkDescriptorSetLayout layout);
    private:
        uint32_t FindAvailablePoolIndex(uint32_t poolIndex);
    private:
        Context *context;
        uint32_t poolIndex = 0;
        uint32_t maxPoolSets;
        std::vector<VkDescriptorPool> pools;
        std::vector<VkDescriptorPoolSize> poolSizes;
        std::vector<uint32_t> poolSetCounts;
    };

    //   ____                            _
    //  / ___|___  _ __ ___  _ __  _   _| |_ ___
    // | |   / _ \| '_ ` _ \| '_ \| | | | __/ _ \
    // | |__| (_) | | | | | | |_) | |_| | ||  __/
    //  \____\___/|_| |_| |_| .__/ \__,_|\__\___|
    //                      |_|
    //

    // ComputePipelineDesc should hold the data for all configurations needed for compute pipeline.
    // When pipeline is initialized, nothing should be changeable (except for dynamic states).
    class ComputePipelineDesc final : public TriviallyConvertible<VkComputePipelineCreateInfo> {
        friend class Pipeline;
    public:
        explicit ComputePipelineDesc();
        virtual ~ComputePipelineDesc() = default;

        ComputePipelineDesc& SetName(const std::string &name) { this->name = name; return *this; }
        const std::string& GetName() const { return name; }

        ComputePipelineDesc& SetPipelineLayout(const PipelineLayoutDesc &layout);
        ComputePipelineDesc& SetComputeShader(Shader* shader);
    private:
        std::string name;
        PipelineLayoutDesc pipelineLayoutDesc;
    };

    //   ____                 _     _
    //  / ___|_ __ __ _ _ __ | |__ (_) ___ ___
    // | |  _| '__/ _` | '_ \| '_ \| |/ __/ __|
    // | |_| | | | (_| | |_) | | | | | (__\__ \
    //  \____|_|  \__,_| .__/|_| |_|_|\___|___/
    //                 |_|

    // GraphicsPipelineDesc should hold the data for all configurations needed for graphics pipeline.
    // When pipeline is initialized, nothing should be changeable (except for dynamic states).
    class GraphicsPipelineDesc final : public TriviallyConvertible<VkGraphicsPipelineCreateInfo> {
        friend class Pipeline;
    public:
        explicit GraphicsPipelineDesc();
        virtual ~GraphicsPipelineDesc() = default;

        GraphicsPipelineDesc& SetName(const std::string &name) { this->name = name; return *this; }
        const std::string& GetName() const { return name; }

        GraphicsPipelineDesc& SetPrimitive(VkPrimitiveTopology primitive, bool dynamic = false);
        GraphicsPipelineDesc& SetCullMode(VkCullModeFlags cullMode, bool dynamic = false);
        GraphicsPipelineDesc& SetFrontFace(VkFrontFace frontFace, bool dynamic = false);
        GraphicsPipelineDesc& SetPolygonMode(VkPolygonMode polygonMode);
        GraphicsPipelineDesc& SetLineWidth(float lineWidth, bool dynamic = false);
        GraphicsPipelineDesc& SetRasterizationDiscard(bool enable, bool dynamic = false);
        GraphicsPipelineDesc& SetDepthTest(VkCompareOp compare);
        GraphicsPipelineDesc& SetDepthClamp(bool enable);
        GraphicsPipelineDesc& SetDepthBias(float depthBiasConstantFactor,
                                           float depthBiasSlopeFactor,
                                           float depthBiasClamp, bool dynamic = false);
        GraphicsPipelineDesc& AddVertexAttrib(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);
        GraphicsPipelineDesc& AddVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
        GraphicsPipelineDesc& SetVertexShader(Shader* shader);
        GraphicsPipelineDesc& SetFragmentShader(Shader* shader);
        GraphicsPipelineDesc& SetPipelineLayout(const PipelineLayoutDesc &layoutBuilder);
        GraphicsPipelineDesc& SetRenderPass(RenderPass *renderPass);
        GraphicsPipelineDesc& SetViewport(const VkExtent2D &extent, bool dynamic = false);
        GraphicsPipelineDesc& SetViewport(const VkViewport &viewport, bool dynamic = false);
        GraphicsPipelineDesc& SetViewportScissors(const std::vector<VkViewport> &viewports,
                                                  const std::vector<VkRect2D> &scissors = {},
                                                  bool dynamic = true);
    private:
        void InitInputAssemblyState();
        void InitRasterizationState();
        void InitColorBlendState();
        void InitDepthStencilState();
        void InitMultisampleState();
        void InitViewportState();
        void InitDynamicState();

    private:
        std::string name = "";

        PipelineLayoutDesc pipelineLayoutDesc;
        SmartPtr<RenderPass> renderPass;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};

        std::vector<VkVertexInputAttributeDescription> vertexAttributes = {};
        std::vector<VkVertexInputBindingDescription> inputBindings = {};
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {};
        std::vector<VkPipelineShaderStageCreateInfo> shaderInfos = {};
        std::vector<VkDynamicState> dynamicStates = {};
        std::vector<VkViewport> viewports = {};
        std::vector<VkRect2D> scissors = {};
    };

    //  ____            _____               _
    // |  _ \ __ _ _   |_   _| __ __ _  ___(_)_ __   __ _
    // | |_) / _` | | | || || '__/ _` |/ __| | '_ \ / _` |
    // |  _ < (_| | |_| || || | | (_| | (__| | | | | (_| |
    // |_| \_\__,_|\__, ||_||_|  \__,_|\___|_|_| |_|\__, |
    //             |___/                            |___/

    // TODO: RayTracingPipelineDesc is currently just a placeholder, its implementation is not targeted as
    // first-class. I will come back to this once other things are settled.
    class RayTracingPipelineDesc final : public TriviallyConvertible<VkRayTracingPipelineCreateInfoKHR> {
        friend class Pipeline;
    public:
        RayTracingPipelineDesc& SetName(const std::string &name) { this->name = name; return *this; }
        const std::string& GetName() const { return name; }
    private:
        std::string name;
    };

    //  ____  _            _ _
    // |  _ \(_)_ __   ___| (_)_ __   ___
    // | |_) | | '_ \ / _ \ | | '_ \ / _ \
    // |  __/| | |_) |  __/ | | | | |  __/
    // |_|   |_| .__/ \___|_|_|_| |_|\___|
    //         |_|

    class Pipeline final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkPipeline> {
    public:
        explicit Pipeline(Context *context, GraphicsPipelineDesc &);
        explicit Pipeline(Context *context, ComputePipelineDesc &);
        explicit Pipeline(Context *context, RayTracingPipelineDesc &);
        virtual ~Pipeline();
        VkPipelineBindPoint Type() const { return bindPoint; }
        PipelineLayout* Layout() const { return layout.get(); }
    private:
        Context* context = nullptr;
        VkPipelineBindPoint bindPoint;
        SmartPtr<PipelineLayout> layout;
    };

} // end of namespace slim

namespace std {
    template <>
    struct hash<slim::DescriptorSetLayoutBinding> {
        size_t operator()(const slim::DescriptorSetLayoutBinding& obj) const {
            size_t hash = 0x0;
            hash = slim::HashCombine(hash, obj.name);
            hash = slim::HashCombine(hash, obj.binding.binding);
            hash = slim::HashCombine(hash, obj.binding.descriptorCount);
            hash = slim::HashCombine(hash, obj.binding.descriptorType);
            hash = slim::HashCombine(hash, obj.binding.stageFlags);
            return hash;
        }
    };

    template <>
    struct hash<slim::PipelineLayoutDesc> {
        size_t operator()(const slim::PipelineLayoutDesc& builder) const {
            size_t hash = 0x0;
            for (const auto &kv : builder.bindings) {
                hash = slim::HashCombine(hash, kv.first);
                for (const auto &binding : kv.second)
                    hash = HashCombine(hash, binding);
            }
            return hash;
        }
    };

    template <>
    struct hash<slim::PipelineLayout> {
        size_t operator()(const slim::PipelineLayout& layout) const {
            return layout.hashValue;
        }
    };
}
#endif // end of SLIM_CORE_PIPELINE_H
