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
#include "core/device.h"
#include "core/renderpass.h"
#include "utility/interface.h"

namespace slim {

    class Pipeline;
    class Descriptor;
    class DescriptorPool;
    class RenderFrame;

    struct VertexAttrib {
        uint32_t location;
        VkFormat format;
        uint32_t offset;
    };

    struct BufferAlloc {
        Buffer* buffer;
        size_t offset = 0;
        size_t size = 0;

        BufferAlloc(Buffer* buffer) : buffer(buffer), offset(0), size(buffer->Size()) {
        }

        BufferAlloc(Buffer* buffer, size_t offset, size_t size) : buffer(buffer), offset(offset), size(size) {
        }
    };

    //  ____  _            _ _            _                            _   ____
    // |  _ \(_)_ __   ___| (_)_ __   ___| |    __ _ _   _  ___  _   _| |_|  _ \  ___  ___  ___
    // | |_) | | '_ \ / _ \ | | '_ \ / _ \ |   / _` | | | |/ _ \| | | | __| | | |/ _ \/ __|/ __|
    // |  __/| | |_) |  __/ | | | | |  __/ |__| (_| | |_| | (_) | |_| | |_| |_| |  __/\__ \ (__
    // |_|   |_| .__/ \___|_|_|_| |_|\___|_____\__,_|\__, |\___/ \__,_|\__|____/ \___||___/\___|
    //         |_|                                   |___/

    struct DescriptorSetLayoutBinding {
        std::string              name;
        uint32_t                 binding;
        VkDescriptorType         descriptorType;
        uint32_t                 descriptorCount;
        VkShaderStageFlags       stageFlags;
        VkDescriptorBindingFlags bindingFlags;
    };

    class PipelineLayoutDesc final {
        friend class PipelineLayout;
        friend class std::hash<PipelineLayoutDesc>;
    public:
        PipelineLayoutDesc& AddPushConstant(const std::string &name, uint32_t offset, uint32_t size,
                                            VkShaderStageFlags stages);
        PipelineLayoutDesc& AddBinding(const std::string &name, uint32_t set, uint32_t binding,
                                       VkDescriptorType descriptorType, VkShaderStageFlags stages, VkDescriptorBindingFlags flags = 0);
        PipelineLayoutDesc& AddBindingArray(const std::string &name, uint32_t set, uint32_t binding, uint32_t count,
                                            VkDescriptorType descriptorType, VkShaderStageFlags stages, VkDescriptorBindingFlags flags = 0);
    private:
        mutable std::multimap<uint32_t, std::vector<DescriptorSetLayoutBinding>> bindings;
        std::vector<std::string> pushConstantNames;
        std::vector<VkPushConstantRange> pushConstantRange;
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
        explicit PipelineLayout(Device *device, const PipelineLayoutDesc &desc);
        virtual ~PipelineLayout();

        bool HasBinding(const std::string &name) const;
        std::tuple<size_t, size_t, VkShaderStageFlags> GetPushConstant(const std::string &name) const;
    private:
        void Init(const PipelineLayoutDesc &desc, std::multimap<uint32_t, std::vector<DescriptorSetLayoutBinding>> &bindings);
    private:
        SmartPtr<Device> device;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::unordered_map<std::string, std::pair<VkDescriptorSetLayout, uint32_t>> mappings;
        std::unordered_map<std::string, std::tuple<uint32_t, uint32_t>> bindings;
        std::unordered_map<std::string, VkPushConstantRange> pushConstants;
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
        explicit Descriptor(DescriptorPool *pool, PipelineLayout *pipelineLayout);
        virtual ~Descriptor();

        // binding a uniform buffer, with offset and size for the target buffer
        void SetUniform(const std::string &name, Buffer *buffer);
        void SetUniform(const std::string &name, const BufferAlloc &bufferAlloc);
        void SetUniforms(const std::string &name, const std::vector<BufferAlloc> &bufferAllocs);

        // binding a dynamic uniform buffer, with offset and size for the target buffer
        // NOTE: size is for each individual uniform element in the buffer
        void SetDynamic(const std::string &name, Buffer *buffer, size_t elemSize);
        void SetDynamic(const std::string &name, const BufferAlloc &bufferAlloc);

        // binding a storage buffer, with offset and size for the target buffer
        void SetStorage(const std::string &name, Buffer *buffer);
        void SetStorage(const std::string &name, const BufferAlloc &bufferAlloc);
        void SetStorages(const std::string &name, const std::vector<BufferAlloc> &bufferAllocs);

        // binding a combined image + sampler
        void SetTexture(const std::string &name, Image *texture, Sampler *sampler);

        // binding a combined image + sampler
        void SetTextures(const std::string &name, const std::vector<Image*> &texture, const std::vector<Sampler*> &sampler);

        // binding image uniform
        void SetImage(const std::string &name, Image *image);

        // binding image array uniform
        void SetImages(const std::string &name, const std::vector<Image*> &images);

        // binding sampler uniform
        void SetSampler(const std::string &name, Sampler *sampler);

        // binding image array uniform
        void SetSamplers(const std::string &name, const std::vector<Sampler*> &samplers);

        bool HasBinding(const std::string &name) const;
        std::tuple<uint32_t, uint32_t> GetBinding(const std::string &name);

        void SetDynamicOffset(const std::string &name, uint32_t offset);
        void SetDynamicOffset(uint32_t set, uint32_t binding, uint32_t offset);
    private:
        void Update();
        std::tuple<VkDescriptorSet, uint32_t, uint32_t> FindDescriptorSet(const std::string &name);
        void SetBuffer(const std::string &name, VkDescriptorType descriptorType, const std::vector<BufferAlloc> &bufferAlloc);
    private:
        SmartPtr<DescriptorPool> pool;
        SmartPtr<PipelineLayout> pipelineLayout;
        std::vector<VkWriteDescriptorSet> writes;
        std::list<std::vector<VkDescriptorBufferInfo>> bufferInfos;
        std::list<std::vector<VkDescriptorImageInfo>> imageInfos;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<std::vector<uint32_t>> dynamicOffsets;
    };

    //  ____                      _       _             ____             _
    // |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __|  _ \ ___   ___ | |
    // | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| |_) / _ \ / _ \| |
    // | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |  |  __/ (_) | (_) | |
    // |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|  |_|   \___/ \___/|_|
    //                             |_|

    class DescriptorPool final : public ReferenceCountable {
        friend class Descriptor;
    public:
        explicit DescriptorPool(Device *device, uint32_t poolSize);
        virtual ~DescriptorPool();
        void Reset();
        VkDescriptorSet Request(VkDescriptorSetLayout layout, uint32_t variableDescriptorCount = 0);
        Device* GetDevice() const { return device; }
    private:
        uint32_t FindAvailablePoolIndex(uint32_t poolIndex);
    private:
        SmartPtr<Device> device;
        uint32_t poolIndex = 0;
        uint32_t maxPoolSets;
        std::vector<VkDescriptorPool> pools;
        std::vector<VkDescriptorPoolSize> poolSizes;
        std::vector<uint32_t> poolSetCounts;
    };

    //  ____  _            _ _              ____
    // |  _ \(_)_ __   ___| (_)_ __   ___  |  _ \  ___  ___  ___
    // | |_) | | '_ \ / _ \ | | '_ \ / _ \ | | | |/ _ \/ __|/ __|
    // |  __/| | |_) |  __/ | | | | |  __/ | |_| |  __/\__ \ (__
    // |_|   |_| .__/ \___|_|_|_| |_|\___| |____/ \___||___/\___|
    //         |_|

    class PipelineDesc {
    public:
        PipelineDesc(VkPipelineBindPoint bindPoint);
        PipelineDesc(const std::string &name, VkPipelineBindPoint bindPoint);

        void Initialize(Device* device);

        PipelineLayout* Layout() const { return pipelineLayout; }
        VkPipelineBindPoint Type() const { return bindPoint; }
        const std::string& GetName() const { return name; }

    protected:
        std::string name = "";
        VkPipelineBindPoint bindPoint;
        SmartPtr<PipelineLayout> pipelineLayout;
        PipelineLayoutDesc pipelineLayoutDesc;
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
    class ComputePipelineDesc final : public PipelineDesc, public TriviallyConvertible<VkComputePipelineCreateInfo> {
        friend class Pipeline;
    public:
        explicit ComputePipelineDesc();
        explicit ComputePipelineDesc(const std::string &name);
        virtual ~ComputePipelineDesc() = default;

        ComputePipelineDesc& SetName(const std::string &name) { this->name = name; return *this; }

        ComputePipelineDesc& SetPipelineLayout(const PipelineLayoutDesc &layout);
        ComputePipelineDesc& SetComputeShader(Shader* shader);
    };

    //   ____                 _     _
    //  / ___|_ __ __ _ _ __ | |__ (_) ___ ___
    // | |  _| '__/ _` | '_ \| '_ \| |/ __/ __|
    // | |_| | | | (_| | |_) | | | | | (__\__ \
    //  \____|_|  \__,_| .__/|_| |_|_|\___|___/
    //                 |_|

    // GraphicsPipelineDesc should hold the data for all configurations needed for graphics pipeline.
    // When pipeline is initialized, nothing should be changeable (except for dynamic states).
    class GraphicsPipelineDesc final : public PipelineDesc, public TriviallyConvertible<VkGraphicsPipelineCreateInfo> {
        friend class Pipeline;
    public:
        explicit GraphicsPipelineDesc();
        explicit GraphicsPipelineDesc(const std::string &name);
        virtual ~GraphicsPipelineDesc() = default;

        GraphicsPipelineDesc& SetName(const std::string &name) { this->name = name; return *this; }

        GraphicsPipelineDesc& SetPrimitive(VkPrimitiveTopology primitive, bool dynamic = false);
        GraphicsPipelineDesc& SetCullMode(VkCullModeFlags cullMode, bool dynamic = false);
        GraphicsPipelineDesc& SetFrontFace(VkFrontFace frontFace, bool dynamic = false);
        GraphicsPipelineDesc& SetPolygonMode(VkPolygonMode polygonMode);
        GraphicsPipelineDesc& SetLineWidth(float lineWidth, bool dynamic = false);
        GraphicsPipelineDesc& SetSampleCount(VkSampleCountFlagBits);
        GraphicsPipelineDesc& SetRasterizationDiscard(bool enable, bool dynamic = false);
        GraphicsPipelineDesc& SetDepthTest(VkCompareOp compare);
        GraphicsPipelineDesc& SetDepthClamp(bool enable);
        GraphicsPipelineDesc& SetDepthBias(float depthBiasConstantFactor,
                                           float depthBiasSlopeFactor,
                                           float depthBiasClamp, bool dynamic = false);
        GraphicsPipelineDesc& AddVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate, const std::vector<VertexAttrib> &attribs);
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
    class RayTracingPipelineDesc final : public PipelineDesc, public TriviallyConvertible<VkRayTracingPipelineCreateInfoKHR> {
        friend class Pipeline;
    public:
        explicit RayTracingPipelineDesc();
        explicit RayTracingPipelineDesc(const std::string &name);
        RayTracingPipelineDesc& SetName(const std::string &name) { this->name = name; return *this; }
    };

    //  ____  _            _ _
    // |  _ \(_)_ __   ___| (_)_ __   ___
    // | |_) | | '_ \ / _ \ | | '_ \ / _ \
    // |  __/| | |_) |  __/ | | | | |  __/
    // |_|   |_| .__/ \___|_|_|_| |_|\___|
    //         |_|

    class Pipeline final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkPipeline> {
    public:
        explicit Pipeline(Device *device, GraphicsPipelineDesc &);
        explicit Pipeline(Device *device, ComputePipelineDesc &);
        explicit Pipeline(Device *device, RayTracingPipelineDesc &);
        virtual ~Pipeline();
        VkPipelineBindPoint Type() const { return bindPoint; }
        PipelineLayout* Layout() const { return layout.get(); }
    private:
        SmartPtr<Device> device = nullptr;
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
            hash = slim::HashCombine(hash, obj.binding);
            hash = slim::HashCombine(hash, obj.descriptorCount);
            hash = slim::HashCombine(hash, obj.descriptorType);
            hash = slim::HashCombine(hash, obj.stageFlags);
            hash = slim::HashCombine(hash, obj.bindingFlags);
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
