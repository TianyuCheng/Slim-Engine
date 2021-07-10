#include "core/debug.h"
#include "core/commands.h"
#include "core/vkutils.h"
#include "core/renderframe.h"

using namespace slim;

//  ____  _            _ _            _                            _   ____
// |  _ \(_)_ __   ___| (_)_ __   ___| |    __ _ _   _  ___  _   _| |_|  _ \  ___  ___  ___
// | |_) | | '_ \ / _ \ | | '_ \ / _ \ |   / _` | | | |/ _ \| | | | __| | | |/ _ \/ __|/ __|
// |  __/| | |_) |  __/ | | | | |  __/ |__| (_| | |_| | (_) | |_| | |_| |_| |  __/\__ \ (__
// |_|   |_| .__/ \___|_|_|_| |_|\___|_____\__,_|\__, |\___/ \__,_|\__|____/ \___||___/\___|
//         |_|                                   |___/

PipelineLayoutDesc& PipelineLayoutDesc::AddBinding(const std::string &name, uint32_t set, uint32_t binding,
                                                   VkDescriptorType descriptorType, VkPipelineShaderStageCreateFlags stages) {
    return AddBindingArray(name, set, binding, 1, descriptorType, stages);
}

PipelineLayoutDesc& PipelineLayoutDesc::AddBindingArray(const std::string &name, uint32_t set, uint32_t binding, uint32_t count,
                                                        VkDescriptorType descriptorType, VkPipelineShaderStageCreateFlags stages) {
    auto it = bindings.find(set);
    if (it == bindings.end()) {
        bindings.insert(std::make_pair(set, std::vector<DescriptorSetLayoutBinding>()));
        it = bindings.find(set);
    }

    it->second.push_back(DescriptorSetLayoutBinding {
        name, VkDescriptorSetLayoutBinding { binding, descriptorType, count, stages, nullptr }
    });

    return *this;
}

//  ____  _            _ _            _                            _
// |  _ \(_)_ __   ___| (_)_ __   ___| |    __ _ _   _  ___  _   _| |_
// | |_) | | '_ \ / _ \ | | '_ \ / _ \ |   / _` | | | |/ _ \| | | | __|
// |  __/| | |_) |  __/ | | | | |  __/ |__| (_| | |_| | (_) | |_| | |_
// |_|   |_| .__/ \___|_|_|_| |_|\___|_____\__,_|\__, |\___/ \__,_|\__|
//         |_|                                   |___/

PipelineLayout::PipelineLayout(Context *context, const PipelineLayoutDesc &desc) : context(context) {
    Init(desc.bindings);
}

void PipelineLayout::Init(std::multimap<uint32_t, std::vector<DescriptorSetLayoutBinding>> &bindings) {
    hashValue = 0x0;

    // iteratively create descriptor set layouts
    for (auto &kv : bindings) {
        hashValue = HashCombine(hashValue, kv.first);
        for (const auto &binding : kv.second)
            hashValue = HashCombine(hashValue, binding);

        // prepare descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        for (const auto &binding : kv.second)
            vkBindings.push_back(binding.binding);

        // create descriptor set layout
        VkDescriptorSetLayout layout;
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = vkBindings.size();
        layoutCreateInfo.pBindings = vkBindings.data();
        layoutCreateInfo.flags = 0;
        layoutCreateInfo.pNext = nullptr;
        ErrorCheck(vkCreateDescriptorSetLayout(context->GetDevice(), &layoutCreateInfo, nullptr, &layout),
                "create descriptor set layout");

        // create a set layout from context
        descriptorSetLayouts.push_back(layout);

        // create a mapping from resource name to descriptor set layout and binding
        for (const auto &binding : kv.second)
            mappings.insert(std::make_pair(binding.name, std::make_pair(layout, binding.binding.binding)));
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    ErrorCheck(vkCreatePipelineLayout(context->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &handle), "create pipeline layout");
}

PipelineLayout::~PipelineLayout() {
    for (auto descriptorSetLayout : descriptorSetLayouts)
        vkDestroyDescriptorSetLayout(context->GetDevice(), descriptorSetLayout, nullptr);

    if (handle) {
        vkDestroyPipelineLayout(context->GetDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}

//  ____                      _       _
// |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __
// | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__|
// | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |
// |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|
//                             |_|

Descriptor::Descriptor(RenderFrame *frame, Pipeline *pipeline)
    : renderFrame(frame), pipeline(pipeline) {
}

Descriptor::~Descriptor() {
}

void Descriptor::Update() {
    VkDevice device = renderFrame->GetContext()->GetDevice();
    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

    writes.clear();
    imageInfos.clear();
    bufferInfos.clear();
}

void Descriptor::SetTexture(const std::string &name, Image *image, Sampler *sampler) {
    auto info = FindDescriptorSet(name);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image->AsTexture();
    imageInfo.sampler = *sampler;
    imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.dstSet = info.first;
    update.dstBinding = info.second;
    update.dstArrayElement = 0;
    update.descriptorCount = 1;
    update.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    update.pImageInfo = &imageInfos.back();
    update.pBufferInfo = nullptr;
    update.pTexelBufferView = nullptr;

    writes.push_back(update);
}

void Descriptor::SetUniform(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    SetBuffer(name, buffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, offset, size);
}

void Descriptor::SetStorage(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    SetBuffer(name, buffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, offset, size);
}

void Descriptor::SetBuffer(const std::string &name, Buffer *buffer, VkDescriptorType descriptorType, size_t offset, size_t size) {
    auto info = FindDescriptorSet(name);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = (VkBuffer) *buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = size == 0 ? buffer->Size() : size;
    bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.dstSet = info.first;
    update.dstBinding = info.second;
    update.dstArrayElement = 0;
    update.descriptorCount = 1;
    update.descriptorType = descriptorType;
    update.pImageInfo = nullptr;
    update.pBufferInfo = &bufferInfos.back();
    update.pTexelBufferView = nullptr;

    writes.push_back(update);
}

//  ____                      _       _             ____             _
// |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __|  _ \ ___   ___ | |
// | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| |_) / _ \ / _ \| |
// | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |  |  __/ (_) | (_) | |
// |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|  |_|   \___/ \___/|_|
//                             |_|

DescriptorPool::DescriptorPool(Context *context, uint32_t size) : context(context), maxPoolSets(size) {
    // initialize descriptor pool sizes
    static std::unordered_map<VkDescriptorType, float> resourceAllocations = {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1.0f },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1.0f },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1.0f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.0f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.0f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 2.0f },
    };

    for (const auto &kv : resourceAllocations) {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = kv.first;
        poolSize.descriptorCount = static_cast<uint32_t>(size * kv.second);
        poolSizes.push_back(poolSize);
    }
}

DescriptorPool::~DescriptorPool() {
    Reset();

    // destroy descriptor pool
    for (const auto &pool : pools) {
        vkDestroyDescriptorPool(context->GetDevice(), pool, nullptr);
    }
}

void DescriptorPool::Reset() {
    // clearing allocated descriptors
    for (const auto &pool : pools) {
        ErrorCheck(vkResetDescriptorPool(context->GetDevice(), pool, (VkDescriptorPoolResetFlags) 0), "reset descriptor pool");
    }

    // reset allocated count
    std::fill(poolSetCounts.begin(), poolSetCounts.end(), 0x0);

    // reset pool index
    poolIndex = 0;
}

uint32_t DescriptorPool::FindAvailablePoolIndex(uint32_t index) {
    // check if we need to allocate new pool
    if (index >= poolSetCounts.size()) {
        // allocate a new descriptor pool
        VkDescriptorPoolCreateInfo createInfo = {};
        createInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = poolSizes.size();
        createInfo.pPoolSizes    = poolSizes.data();
        createInfo.maxSets       = maxPoolSets;
        createInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

        VkDescriptorPool pool = VK_NULL_HANDLE;
        ErrorCheck(vkCreateDescriptorPool(context->GetDevice(), &createInfo, nullptr, &pool), "create new descriptor pool");

        pools.push_back(pool);
        poolSetCounts.push_back(0);
        return pools.size() - 1;
    }

    if (poolSetCounts[index] < maxPoolSets) {
        return index;
    }

    return FindAvailablePoolIndex(index + 1);
}

VkDescriptorSet DescriptorPool::Request(VkDescriptorSetLayout layout) {
    poolIndex = FindAvailablePoolIndex(poolIndex);

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = pools[poolIndex];
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layout;
    ErrorCheck(vkAllocateDescriptorSets(context->GetDevice(), &allocInfo, &descriptorSet), "allocate a descriptor set");

    // increment allocated set counter in the pool
    poolSetCounts[poolIndex]++;
    return descriptorSet;
}
std::pair<VkDescriptorSet, uint32_t> Descriptor::FindDescriptorSet(const std::string &name) {
    auto indexOf = [](const std::vector<VkDescriptorSetLayout> &layouts, VkDescriptorSetLayout target) {
        for (uint32_t i = 0; i < layouts.size(); i++)
            if (layouts[i] == target)
                return i;
        return 0xffffffff;
    };

    VkDescriptorSet descriptorSet;

    auto it = pipeline->Layout()->mappings.find(name);
    if (it == pipeline->Layout()->mappings.end()) {
        throw std::runtime_error("Failed to find binding with name: " + name);
    }

    VkDescriptorSetLayout layout = it->second.first;
    uint32_t set = indexOf(pipeline->Layout()->descriptorSetLayouts, layout);
    uint32_t binding = it->second.second;

    // augment the descriptor sets array
    while (descriptorSets.size() <= set)
        descriptorSets.push_back(VK_NULL_HANDLE);

    if (descriptorSets[set] == VK_NULL_HANDLE) {
        descriptorSets[set] = renderFrame->RequestDescriptorSet(layout);
    }

    descriptorSet = descriptorSets[set];
    return std::make_pair(descriptorSet, binding);
}


//   ____                            _
//  / ___|___  _ __ ___  _ __  _   _| |_ ___
// | |   / _ \| '_ ` _ \| '_ \| | | | __/ _ \
// | |__| (_) | | | | | | |_) | |_| | ||  __/
//  \____\___/|_| |_| |_| .__/ \__,_|\__\___|
//                      |_|
//

ComputePipelineDesc::ComputePipelineDesc() {
    handle.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    handle.flags = 0;
    handle.pNext = nullptr;
}

ComputePipelineDesc& ComputePipelineDesc::SetPipelineLayout(const PipelineLayoutDesc &layoutDesc) {
    pipelineLayoutDesc = layoutDesc;
    return *this;
}

ComputePipelineDesc& ComputePipelineDesc::SetComputeShader(Shader* shader) {
    handle.stage = shader->GetInfo();
    return *this;
}

//   ____                 _     _
//  / ___|_ __ __ _ _ __ | |__ (_) ___ ___
// | |  _| '__/ _` | '_ \| '_ \| |/ __/ __|
// | |_| | | | (_| | |_) | | | | | (__\__ \
//  \____|_|  \__,_| .__/|_| |_|_|\___|___/
//                 |_|

GraphicsPipelineDesc::GraphicsPipelineDesc() {
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

GraphicsPipelineDesc& GraphicsPipelineDesc::SetRasterizationDiscard(bool enable, bool dynamic) {
    rasterizationStateCreateInfo.rasterizerDiscardEnable = enable;
    if (dynamic) dynamicStates.push_back(VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT);
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

GraphicsPipelineDesc& GraphicsPipelineDesc::AddVertexAttrib(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) {
    vertexAttributes.push_back(VkVertexInputAttributeDescription {
        location, binding, format, offset
    });
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::AddVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
    inputBindings.push_back(VkVertexInputBindingDescription {
        binding, stride, inputRate
    });
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetVertexShader(Shader* shader) {
    shaderInfos.push_back(shader->GetInfo());
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetFragmentShader(Shader* shader) {
    shaderInfos.push_back(shader->GetInfo());
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

GraphicsPipelineDesc& GraphicsPipelineDesc::ViewportScissors(const std::vector<VkViewport> &viewports,
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

Pipeline::Pipeline(Context *context, GraphicsPipelineDesc &desc) : context(context), bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS) {
    layout = SlimPtr<PipelineLayout>(context, desc.pipelineLayoutDesc);

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};

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
    desc.handle.stageCount = desc.shaderInfos.size();
    desc.handle.pStages = desc.shaderInfos.data();
    desc.handle.pNext = VK_NULL_HANDLE;
    desc.handle.layout = *layout;

    ErrorCheck(vkCreateGraphicsPipelines(context->GetDevice(), VK_NULL_HANDLE, 1, &desc.handle, nullptr, &handle), "create graphics pipeline");
}

Pipeline::Pipeline(Context *context, ComputePipelineDesc &desc) : context(context), bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE){
    layout = SlimPtr<PipelineLayout>(context, desc.pipelineLayoutDesc);

    desc.handle.basePipelineHandle = handle;
    desc.handle.basePipelineIndex = 0;
    desc.handle.flags = 0;
    desc.handle.layout = *layout;

    ErrorCheck(vkCreateComputePipelines(context->GetDevice(), VK_NULL_HANDLE, 1, &desc.handle, nullptr, &handle), "create compute pipeline");
}

Pipeline::Pipeline(Context *context, RayTracingPipelineDesc &) : context(context), bindPoint(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR) {
    throw std::runtime_error("raytracing pipeline not implemented!");
}

Pipeline::~Pipeline() {
    layout.reset();

    if (handle) {
        vkDestroyPipeline(context->GetDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
