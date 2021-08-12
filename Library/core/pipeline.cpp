#include "core/debug.h"
#include "core/vkutils.h"
#include "core/commands.h"
#include "core/descriptor.h"
#include "core/renderframe.h"

using namespace slim;

//  ____  _            _ _            _                            _   ____
// |  _ \(_)_ __   ___| (_)_ __   ___| |    __ _ _   _  ___  _   _| |_|  _ \  ___  ___  ___
// | |_) | | '_ \ / _ \ | | '_ \ / _ \ |   / _` | | | |/ _ \| | | | __| | | |/ _ \/ __|/ __|
// |  __/| | |_) |  __/ | | | | |  __/ |__| (_| | |_| | (_) | |_| | |_| |_| |  __/\__ \ (__
// |_|   |_| .__/ \___|_|_|_| |_|\___|_____\__,_|\__, |\___/ \__,_|\__|____/ \___||___/\___|
//         |_|                                   |___/

PipelineLayoutDesc& PipelineLayoutDesc::AddPushConstant(const std::string &name, uint32_t offset, uint32_t size,
                                                        VkShaderStageFlags stages) {
    pushConstantNames.push_back(name);
    pushConstantRange.push_back(VkPushConstantRange {
        stages, offset, size
    });
    return *this;
}

PipelineLayoutDesc& PipelineLayoutDesc::AddBinding(const std::string &name, uint32_t set, uint32_t binding,
                                                   VkDescriptorType descriptorType, VkShaderStageFlags stages, VkDescriptorBindingFlags flags) {
    return AddBindingArray(name, set, binding, 1, descriptorType, stages, flags);
}

PipelineLayoutDesc& PipelineLayoutDesc::AddBindingArray(const std::string &name, uint32_t set, uint32_t binding, uint32_t count,
                                                        VkDescriptorType descriptorType, VkShaderStageFlags stages, VkDescriptorBindingFlags flags) {
    auto it = bindings.find(set);
    if (it == bindings.end()) {
        bindings.insert(std::make_pair(set, std::vector<DescriptorSetLayoutBinding>()));
        it = bindings.find(set);
    }

    it->second.push_back(DescriptorSetLayoutBinding {
        name, set, binding, descriptorType, count, stages, flags,
    });

    return *this;
}

//  ____  _            _ _            _                            _
// |  _ \(_)_ __   ___| (_)_ __   ___| |    __ _ _   _  ___  _   _| |_
// | |_) | | '_ \ / _ \ | | '_ \ / _ \ |   / _` | | | |/ _ \| | | | __|
// |  __/| | |_) |  __/ | | | | |  __/ |__| (_| | |_| | (_) | |_| | |_
// |_|   |_| .__/ \___|_|_|_| |_|\___|_____\__,_|\__, |\___/ \__,_|\__|
//         |_|                                   |___/

PipelineLayout::PipelineLayout(Device *device, const PipelineLayoutDesc &desc) : device(device) {
    Init(desc, desc.bindings);

    for (uint32_t i = 0; i < desc.pushConstantNames.size(); i++) {
        pushConstants.insert(std::make_pair(
            desc.pushConstantNames[i],
            desc.pushConstantRange[i]
        ));
    }
}

void PipelineLayout::Init(const PipelineLayoutDesc &desc, std::multimap<uint32_t, std::vector<DescriptorSetLayoutBinding>> &bindings) {
    hashValue = 0x0;

    std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandles = {};

    // iteratively create descriptor set layouts
    for (auto &kv : bindings) {
        hashValue = HashCombine(hashValue, kv.first);
        for (const auto &binding : kv.second)
            hashValue = HashCombine(hashValue, binding);

        bool needDescriptorFlags = false;

        const std::vector<DescriptorSetLayoutBinding>& setBindings = kv.second;

        // prepare descriptor set layout
        std::vector<VkDescriptorBindingFlags> bindingFlags;
        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        for (const auto &binding : setBindings) {
            bindingFlags.push_back(binding.bindingFlags);
            vkBindings.push_back(VkDescriptorSetLayoutBinding {
                binding.binding,
                binding.descriptorType,
                binding.descriptorCount,
                binding.stageFlags,
                nullptr // immutable samplers
            });
            if (binding.bindingFlags) needDescriptorFlags = true;
        }

        // create descriptor set layout
        VkDescriptorSetLayout layout;
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = vkBindings.size();
        layoutCreateInfo.pBindings = vkBindings.data();
        layoutCreateInfo.flags = 0;
        layoutCreateInfo.pNext = nullptr;

        // prepare descriptor binding flags
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo = {};
        if (needDescriptorFlags) {
            bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            bindingFlagsCreateInfo.bindingCount = bindingFlags.size();
            bindingFlagsCreateInfo.pBindingFlags = bindingFlags.data();
            bindingFlagsCreateInfo.pNext = layoutCreateInfo.pNext;
            layoutCreateInfo.pNext = &bindingFlagsCreateInfo;
        }

        ErrorCheck(vkCreateDescriptorSetLayout(*device, &layoutCreateInfo, nullptr, &layout),
                "create descriptor set layout");

        // create a set layout from device
        descriptorSetLayoutHandles.push_back(layout);
        descriptorSetLayouts.push_back(DescriptorSetLayout {
            layout, setBindings
        });

        // create a mapping from resource name to descriptor set layout and binding
        uint32_t setIndex = descriptorSetLayouts.size() - 1;
        for (uint32_t bindingIndex = 0; bindingIndex < setBindings.size(); bindingIndex++) {
            this->bindings.insert(std::make_pair(
                    setBindings[bindingIndex].name,
                    std::make_tuple(setIndex, bindingIndex)));
        }
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayoutHandles.size();
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayoutHandles.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = desc.pushConstantRange.size();
    pipelineLayoutCreateInfo.pPushConstantRanges = desc.pushConstantRange.data();
    ErrorCheck(vkCreatePipelineLayout(*device, &pipelineLayoutCreateInfo, nullptr, &handle), "create pipeline layout");
}

PipelineLayout::~PipelineLayout() {
    for (auto descriptorSetLayout : descriptorSetLayouts)
        vkDestroyDescriptorSetLayout(*device, descriptorSetLayout.layout, nullptr);

    if (handle) {
        vkDestroyPipelineLayout(*device, handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}

bool PipelineLayout::HasBinding(const std::string &name) const {
    return bindings.find(name) != bindings.end();
}

const DescriptorSetLayout& PipelineLayout::GetSetBinding(const std::string &name, uint32_t& indexAccessor) const {
    const auto it = bindings.find(name);

    #ifndef NDEBUG
    if (it == bindings.end()) {
        throw std::runtime_error("[PipelineLayout] Failed to find binding == " + name);
    }
    #endif

    auto [setIndex, bindingIndex] = it->second;
    indexAccessor = bindingIndex;   // update reference value
    return descriptorSetLayouts[setIndex];
}

const VkPushConstantRange& PipelineLayout::GetPushConstant(const std::string &name) const {
    auto it = pushConstants.find(name);

    #ifndef NDEBUG
    if (it == pushConstants.end()) {
        throw std::runtime_error("[PipelineLayout] Failed to find push constant == " + name);
    }
    #endif

    return it->second;
}

//  ____  _            _ _              ____
// |  _ \(_)_ __   ___| (_)_ __   ___  |  _ \  ___  ___  ___
// | |_) | | '_ \ / _ \ | | '_ \ / _ \ | | | |/ _ \/ __|/ __|
// |  __/| | |_) |  __/ | | | | |  __/ | |_| |  __/\__ \ (__
// |_|   |_| .__/ \___|_|_|_| |_|\___| |____/ \___||___/\___|
//         |_|

PipelineDesc::PipelineDesc(VkPipelineBindPoint bindPoint) : bindPoint(bindPoint) {

}

PipelineDesc::PipelineDesc(const std::string &name, VkPipelineBindPoint bindPoint) : name(name), bindPoint(bindPoint) {

}

void PipelineDesc::Initialize(Device* device) {
    if (!pipelineLayout) {
        pipelineLayout = SlimPtr<PipelineLayout>(device, pipelineLayoutDesc);
    }
}

//   ____                            _
//  / ___|___  _ __ ___  _ __  _   _| |_ ___
// | |   / _ \| '_ ` _ \| '_ \| | | | __/ _ \
// | |__| (_) | | | | | | |_) | |_| | ||  __/
//  \____\___/|_| |_| |_| .__/ \__,_|\__\___|
//                      |_|
//

ComputePipelineDesc::ComputePipelineDesc() : PipelineDesc(VK_PIPELINE_BIND_POINT_COMPUTE) {
    handle.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    handle.flags = 0;
    handle.pNext = nullptr;
}

ComputePipelineDesc::ComputePipelineDesc(const std::string &name) : PipelineDesc(name, VK_PIPELINE_BIND_POINT_COMPUTE) {
    handle.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    handle.flags = 0;
    handle.pNext = nullptr;
}

ComputePipelineDesc& ComputePipelineDesc::SetPipelineLayout(const PipelineLayoutDesc &layoutDesc) {
    pipelineLayoutDesc = layoutDesc;
    return *this;
}

ComputePipelineDesc& ComputePipelineDesc::SetComputeShader(Shader* shader) {
    computeShader = shader;
    return *this;
}

//   ____                 _     _
//  / ___|_ __ __ _ _ __ | |__ (_) ___ ___
// | |  _| '__/ _` | '_ \| '_ \| |/ __/ __|
// | |_| | | | (_| | |_) | | | | | (__\__ \
//  \____|_|  \__,_| .__/|_| |_|_|\___|___/
//                 |_|

GraphicsPipelineDesc::GraphicsPipelineDesc() : PipelineDesc(VK_PIPELINE_BIND_POINT_GRAPHICS) {
    handle = {};
    InitInputAssemblyState();
    InitRasterizationState();
    InitColorBlendState();
    InitDepthStencilState();
    InitMultisampleState();
    InitDynamicState();
}

GraphicsPipelineDesc::GraphicsPipelineDesc(const std::string &name) : PipelineDesc(name, VK_PIPELINE_BIND_POINT_GRAPHICS) {
    handle = {};
    InitInputAssemblyState();
    InitRasterizationState();
    InitColorBlendState();
    InitDepthStencilState();
    InitMultisampleState();
    InitDynamicState();
}

void GraphicsPipelineDesc::InitInputAssemblyState() {
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
}

void GraphicsPipelineDesc::InitRasterizationState() {
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
}

void GraphicsPipelineDesc::InitColorBlendState() {
    // initialize a default color blend attachment state
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachments.push_back(colorBlendAttachment);
    // ----------------------------------------------------------------------------------------
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
}

void GraphicsPipelineDesc::InitMultisampleState() {
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
}

void GraphicsPipelineDesc::InitDynamicState() {
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
}

void GraphicsPipelineDesc::InitDepthStencilState() {
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.depthTestEnable = false;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.depthWriteEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.pNext = nullptr;
    depthStencilStateCreateInfo.flags = 0;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetPrimitive(VkPrimitiveTopology primitive, bool dynamic) {
    inputAssemblyStateCreateInfo.topology = primitive;
    if (dynamic) dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetCullMode(VkCullModeFlags cullMode, bool dynamic) {
    rasterizationStateCreateInfo.cullMode = cullMode;
    if (dynamic) dynamicStates.push_back(VK_DYNAMIC_STATE_CULL_MODE_EXT);
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetFrontFace(VkFrontFace frontFace, bool dynamic) {
    rasterizationStateCreateInfo.frontFace = frontFace;
    if (dynamic) dynamicStates.push_back(VK_DYNAMIC_STATE_FRONT_FACE_EXT);
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetPolygonMode(VkPolygonMode polygonMode) {
    rasterizationStateCreateInfo.polygonMode = polygonMode;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetLineWidth(float lineWidth, bool dynamic) {
    rasterizationStateCreateInfo.lineWidth = lineWidth;
    if (dynamic) dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetSampleCount(VkSampleCountFlagBits samples) {
    multisampleStateCreateInfo.rasterizationSamples = samples;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetRasterizationDiscard(bool enable, bool dynamic) {
    rasterizationStateCreateInfo.rasterizerDiscardEnable = enable;
    if (dynamic) dynamicStates.push_back(VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT);
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetDepthTest(VkCompareOp compare, bool writeEnabled) {
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = writeEnabled;
    depthStencilStateCreateInfo.depthCompareOp = compare;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetDepthClamp(bool enable) {
    rasterizationStateCreateInfo.depthClampEnable = enable;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetDepthBias(float depthBiasConstantFactor,
                                                         float depthBiasSlopeFactor,
                                                         float depthBiasClamp, bool dynamic) {
    rasterizationStateCreateInfo.depthBiasEnable = true;
    rasterizationStateCreateInfo.depthBiasConstantFactor = depthBiasConstantFactor;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
    rasterizationStateCreateInfo.depthBiasClamp = depthBiasClamp;
    if (dynamic) dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::AddVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate,
                                                             const std::vector<VertexAttrib> &attribs) {
    inputBindings.push_back(VkVertexInputBindingDescription {
        binding, stride, inputRate
    });

    for (const VertexAttrib &attrib : attribs) {
        vertexAttributes.push_back(VkVertexInputAttributeDescription {
            attrib.location, binding, attrib.format, attrib.offset
        });
    }

    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetVertexShader(Shader* shader) {
    vertexShader = shader;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetFragmentShader(Shader* shader) {
    fragmentShader = shader;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetPipelineLayout(const PipelineLayoutDesc &layoutDesc) {
    pipelineLayoutDesc = layoutDesc;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetRenderPass(RenderPass *rp) {
    renderPass.reset(rp);
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetViewport(const VkExtent2D &extent, bool dynamic) {
    return SetViewport(VkViewport {
        0.0f, 0.0f,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f, 1.0f }, dynamic);
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetViewport(const VkViewport &viewport, bool dynamic) {
    return SetViewportScissors({ viewport }, {}, dynamic);
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetViewportScissors(const std::vector<VkViewport> &viewports,
                                                                const std::vector<VkRect2D> &scissors,
                                                                bool dynamic) {
    this->viewports.clear();
    this->scissors.clear();

    // viewports (doing a manual flip)
    for (const auto &viewport : viewports) {
        this->viewports.push_back(VkViewport {
            static_cast<float>(viewport.x),
            static_cast<float>(viewport.height - viewport.y),
            static_cast<float>(viewport.width),
            static_cast<float>(-viewport.height),
            viewport.minDepth, viewport.maxDepth
        });
    }

    // scissors
    for (const auto &scissor : scissors) {
        this->scissors.push_back(scissor);
    }

    // adding a default scissor
    if (this->scissors.size() == 0) {
        const auto &viewport = viewports.back();
        this->scissors.push_back(VkRect2D {
            { static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y) },
            { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) }
        });
    }

    if (dynamic) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    }

    return *this;
}

//  ____  _            _ _
// |  _ \(_)_ __   ___| (_)_ __   ___
// | |_) | | '_ \ / _ \ | | '_ \ / _ \
// |  __/| | |_) |  __/ | | | | |  __/
// |_|   |_| .__/ \___|_|_|_| |_|\___|
//         |_|

Pipeline::Pipeline(Device *device, GraphicsPipelineDesc &desc) : device(device), bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS) {
    #ifndef NDEBUG
    assert(desc.name.length() > 0 && "PipelineDesc must have a name!");
    assert(desc.renderPass && "PipelineDesc must have a valid renderPass!");
    #endif

    desc.Initialize(device);
    layout = desc.Layout();

    // shader infos
    std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos = {};
    if (desc.vertexShader) shaderCreateInfos.push_back(desc.vertexShader->GetInfo());
    if (desc.fragmentShader) shaderCreateInfos.push_back(desc.fragmentShader->GetInfo());

    // vertex attributes & input bindings
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = desc.vertexAttributes.size();
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = desc.vertexAttributes.data();
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = desc.inputBindings.size();
    vertexInputStateCreateInfo.pVertexBindingDescriptions = desc.inputBindings.data();
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.pNext = nullptr;

    // color blend state
    desc.colorBlendStateCreateInfo.attachmentCount = desc.colorBlendAttachments.size();
    desc.colorBlendStateCreateInfo.pAttachments = desc.colorBlendAttachments.data();

    // viewport & scissors
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = desc.viewports.size();
    viewportStateCreateInfo.pViewports = desc.viewports.data();
    viewportStateCreateInfo.scissorCount = desc.scissors.size();
    viewportStateCreateInfo.pScissors = desc.scissors.data();
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.pNext = nullptr;

    desc.handle.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    desc.handle.basePipelineHandle = handle;
    desc.handle.basePipelineIndex = 0;
    desc.handle.pColorBlendState = &desc.colorBlendStateCreateInfo;
    desc.handle.pDepthStencilState = &desc.depthStencilStateCreateInfo;
    desc.handle.pDynamicState = &desc.dynamicStateCreateInfo;
    desc.handle.pInputAssemblyState = &desc.inputAssemblyStateCreateInfo;
    desc.handle.pMultisampleState = &desc.multisampleStateCreateInfo;
    desc.handle.pRasterizationState = &desc.rasterizationStateCreateInfo;
    desc.handle.pTessellationState = nullptr;
    desc.handle.pVertexInputState = &vertexInputStateCreateInfo;
    desc.handle.pViewportState = &viewportStateCreateInfo;
    desc.handle.renderPass = *desc.renderPass;
    desc.handle.subpass = 0;
    desc.handle.stageCount = shaderCreateInfos.size();
    desc.handle.pStages = shaderCreateInfos.data();
    desc.handle.pNext = VK_NULL_HANDLE;
    desc.handle.layout = *layout;

    ErrorCheck(vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &desc.handle, nullptr, &handle), "create graphics pipeline");
}

Pipeline::Pipeline(Device *device, ComputePipelineDesc &desc) : device(device), bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE) {
    desc.Initialize(device);
    layout = desc.Layout();

    // shader infos
    VkPipelineShaderStageCreateInfo shaderCreateInfo = desc.computeShader->GetInfo();

    desc.handle.basePipelineHandle = handle;
    desc.handle.basePipelineIndex = 0;
    desc.handle.flags = 0;
    desc.handle.layout = *layout;
    desc.handle.stage = shaderCreateInfo;

    ErrorCheck(vkCreateComputePipelines(*device, VK_NULL_HANDLE, 1, &desc.handle, nullptr, &handle), "create compute pipeline");
}

Pipeline::Pipeline(Device *device, RayTracingPipelineDesc &) : device(device), bindPoint(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR) {
    throw std::runtime_error("raytracing pipeline not implemented!");
}

Pipeline::~Pipeline() {
    layout.reset();

    if (handle) {
        vkDestroyPipeline(*device, handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
