#ifndef SLIM_CORE_PIPELINE_H
#define SLIM_CORE_PIPELINE_H

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

#include "core/vulkan.h"
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
        uint32_t                 set;
        uint32_t                 binding;
        VkDescriptorType         descriptorType;
        uint32_t                 descriptorCount;
        VkShaderStageFlags       stageFlags;
        VkDescriptorBindingFlags bindingFlags;
    };

    struct DescriptorSetLayout {
        VkDescriptorSetLayout layout;
        std::vector<DescriptorSetLayoutBinding> bindings;
    };

    struct SetBinding {
        uint32_t set;
        uint32_t binding;
    };

    struct Range {
        uint32_t offset;
        uint32_t size;
    };

    class PipelineLayoutDesc final {
        friend class PipelineLayout;
        friend class std::hash<PipelineLayoutDesc>;
    public:

        PipelineLayoutDesc& AddPushConstant(
                const std::string &name,
                const Range& range,
                VkShaderStageFlags stages);

        PipelineLayoutDesc& AddBinding(
                const std::string &name,
                const SetBinding& binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stages,
                VkDescriptorBindingFlags flags = 0);

        PipelineLayoutDesc& AddBindingArray(
                const std::string &name,
                const SetBinding& binding,
                uint32_t count,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stages,
                VkDescriptorBindingFlags flags = 0);

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
        friend class std::hash<PipelineLayout>;
    public:
        explicit PipelineLayout(Device *device, const PipelineLayoutDesc &desc);
        virtual ~PipelineLayout();

        bool HasBinding(const std::string &name) const;
        const DescriptorSetLayout& GetSetBindings(uint32_t set) const { return descriptorSetLayouts[set]; }
        const DescriptorSetLayout& GetSetBinding(const std::string &name, uint32_t& indexAccessor) const;
        const VkPushConstantRange& GetPushConstant(const std::string &name) const;
    private:
        void Init(const PipelineLayoutDesc &desc, std::multimap<uint32_t, std::vector<DescriptorSetLayoutBinding>> &bindings);
    private:
        SmartPtr<Device> device;
        std::vector<DescriptorSetLayout> descriptorSetLayouts;
        std::unordered_map<std::string, std::tuple<uint32_t, uint32_t>> bindings;
        std::unordered_map<std::string, VkPushConstantRange> pushConstants;
        size_t hashValue;
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

    private:
        SmartPtr<Shader> computeShader;
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
        GraphicsPipelineDesc& SetDepthTest(VkCompareOp compare, bool writeEnabled = true);
        GraphicsPipelineDesc& SetDepthWrite(bool value);
        GraphicsPipelineDesc& SetDepthClamp(bool enable);
        GraphicsPipelineDesc& SetDepthBias(float depthBiasConstantFactor,
                                           float depthBiasSlopeFactor,
                                           float depthBiasClamp, bool dynamic = false);
        GraphicsPipelineDesc& SetStencilTest(const VkStencilOpState& front, const VkStencilOpState& back);
        GraphicsPipelineDesc& SetBlendState(uint32_t index, const VkPipelineColorBlendAttachmentState& blendState);
        GraphicsPipelineDesc& SetDefaultBlendState(uint32_t index);
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
        SmartPtr<Shader> vertexShader;
        SmartPtr<Shader> fragmentShader;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};

        std::vector<VkVertexInputAttributeDescription> vertexAttributes = {};
        std::vector<VkVertexInputBindingDescription> inputBindings = {};
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {};
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

    class RayTracingPipelineDesc final : public PipelineDesc, public TriviallyConvertible<VkRayTracingPipelineCreateInfoKHR> {
        friend class Pipeline;
    public:
        explicit RayTracingPipelineDesc();
        explicit RayTracingPipelineDesc(const std::string &name);
        RayTracingPipelineDesc& SetName(const std::string &name) { this->name = name; return *this; }
        RayTracingPipelineDesc& SetPipelineLayout(const PipelineLayoutDesc &layout);

        // settings
        RayTracingPipelineDesc& SetMaxRayRecursionDepth(int depth = 1);

        // shader table
        RayTracingPipelineDesc& SetRayGenShader(Shader* shader);
        RayTracingPipelineDesc& SetMissShader(Shader* shader);
        RayTracingPipelineDesc& SetAnyHitShader(Shader* shader);
        RayTracingPipelineDesc& SetClosestHitShader(Shader* shader);
        RayTracingPipelineDesc& SetAnyHitShader(Shader* aHitShader, Shader* isectShader);
        RayTracingPipelineDesc& SetClosestHitShader(Shader* cHitShader, Shader* isectShader);

    private:
        uint32_t FindShader(Shader* shader);

    private:
        VkPipelineLibraryCreateInfoKHR pipelineLibraryCreateInfo = {};
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> rayGenCreateInfos = {};
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> missCreateInfos = {};
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> hitCreateInfos = {};
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> callableCreateInfos = {};
        std::vector<SmartPtr<Shader>> shaders = {};
        std::vector<VkPipelineShaderStageCreateInfo> shaderInfos = {};
    };

    //  ____  _            _ _
    // |  _ \(_)_ __   ___| (_)_ __   ___
    // | |_) | | '_ \ / _ \ | | '_ \ / _ \
    // |  __/| | |_) |  __/ | | | | |  __/
    // |_|   |_| .__/ \___|_|_|_| |_|\___|
    //         |_|

    class Pipeline final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkPipeline> {
    public:
        explicit Pipeline(Device *device, GraphicsPipelineDesc &, uint32_t subpass = 0);
        explicit Pipeline(Device *device, ComputePipelineDesc &);
        explicit Pipeline(Device *device, RayTracingPipelineDesc &);
        virtual ~Pipeline();
        VkPipelineBindPoint Type() const { return bindPoint; }
        PipelineLayout* Layout() const { return layout.get(); }

        VkStridedDeviceAddressRegionKHR* GetRayGenRegion() { return &raygen; }
        VkStridedDeviceAddressRegionKHR* GetMissRegion() { return &miss; }
        VkStridedDeviceAddressRegionKHR* GetHitRegion() { return &hit; }
        VkStridedDeviceAddressRegionKHR* GetCallableRegion() { return &callable; }

        void SetName(const std::string& name) const;

    private:
        SmartPtr<Device> device = nullptr;
        VkPipelineBindPoint bindPoint;
        SmartPtr<PipelineLayout> layout;

        SmartPtr<Buffer> sbtBuffer;
        VkStridedDeviceAddressRegionKHR raygen;
        VkStridedDeviceAddressRegionKHR miss;
        VkStridedDeviceAddressRegionKHR hit;
        VkStridedDeviceAddressRegionKHR callable;
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
