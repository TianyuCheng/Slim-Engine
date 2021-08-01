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

PipelineLayoutDesc& PipelineLayoutDesc::AddPushConstant(const std::string &name, uint32_t offset, uint32_t size,
                                                        VkShaderStageFlags stages) {
    pushConstantNames.push_back(name);
    pushConstantRange.push_back(VkPushConstantRange {
        stages, offset, size
    });
    return *this;
}

PipelineLayoutDesc& PipelineLayoutDesc::AddBinding(const std::string &name, uint32_t set, uint32_t binding,
                                                   VkDescriptorType descriptorType, VkShaderStageFlags stages) {
    return AddBindingArray(name, set, binding, 1, descriptorType, stages);
}

PipelineLayoutDesc& PipelineLayoutDesc::AddBindingArray(const std::string &name, uint32_t set, uint32_t binding, uint32_t count,
                                                        VkDescriptorType descriptorType, VkShaderStageFlags stages) {
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
        ErrorCheck(vkCreateDescriptorSetLayout(*device, &layoutCreateInfo, nullptr, &layout),
                "create descriptor set layout");

        // create a set layout from device
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
    pipelineLayoutCreateInfo.pushConstantRangeCount = desc.pushConstantRange.size();
    pipelineLayoutCreateInfo.pPushConstantRanges = desc.pushConstantRange.data();
    ErrorCheck(vkCreatePipelineLayout(*device, &pipelineLayoutCreateInfo, nullptr, &handle), "create pipeline layout");
}

PipelineLayout::~PipelineLayout() {
    for (auto descriptorSetLayout : descriptorSetLayouts)
        vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);

    if (handle) {
        vkDestroyPipelineLayout(*device, handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}

bool PipelineLayout::HasBinding(const std::string &name) const {
    return mappings.find(name) != mappings.end();
}

std::tuple<size_t, size_t, VkShaderStageFlags> PipelineLayout::GetPushConstant(const std::string &name) const {
    auto it = pushConstants.find(name);

    #ifndef NDEBUG
    if (it == pushConstants.end()) {
        throw std::runtime_error("[PipelineLayout] Failed to find push constant == " + name);
    }
    #endif

    const auto &pushConstant = it->second;
    return std::make_tuple(pushConstant.offset, pushConstant.size, pushConstant.stageFlags);
}

//  ____                      _       _
// |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __
// | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__|
// | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |
// |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|
//                             |_|

Descriptor::Descriptor(DescriptorPool *pool, PipelineLayout *pipelineLayout)
    : pool(pool), pipelineLayout(pipelineLayout) {
}

Descriptor::~Descriptor() {
}

void Descriptor::Update() {
    VkDevice device = *pool->GetDevice();
    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

    writes.clear();
    imageInfos.clear();
    bufferInfos.clear();
}

bool Descriptor::HasBinding(const std::string &name) const {
    return pipelineLayout->HasBinding(name);
}

std::tuple<uint32_t, uint32_t> Descriptor::GetBinding(const std::string &name) {
    auto [_, set, binding] = FindDescriptorSet(name);
    return std::make_tuple(set, binding);
}

void Descriptor::SetTexture(const std::string &name, Image *image, Sampler *sampler) {
    auto [descriptorSet, _, binding] = FindDescriptorSet(name);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image->AsTexture();
    imageInfo.sampler = *sampler;
    imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.dstSet = descriptorSet;
    update.dstBinding = binding;
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

void Descriptor::SetDynamic(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    SetBuffer(name, buffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, offset, size);
}

void Descriptor::SetStorage(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    SetBuffer(name, buffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, offset, size);
}

void Descriptor::SetBuffer(const std::string &name, Buffer *buffer, VkDescriptorType descriptorType, size_t offset, size_t size) {
    buffer->Flush();

    auto [descriptorSet, _, binding] = FindDescriptorSet(name);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = (VkBuffer) *buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = size == 0 ? buffer->Size() : size;
    bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.dstSet = descriptorSet;
    update.dstBinding = binding;
    update.dstArrayElement = 0;
    update.descriptorCount = 1;
    update.descriptorType = descriptorType;
    update.pImageInfo = nullptr;
    update.pBufferInfo = &bufferInfos.back();
    update.pTexelBufferView = nullptr;

    writes.push_back(update);
}

void Descriptor::SetDynamicOffset(const std::string &name, uint32_t offset) {
    auto [_, set, binding] = FindDescriptorSet(name);
    SetDynamicOffset(set, binding, offset);
}

void Descriptor::SetDynamicOffset(uint32_t set, uint32_t binding, uint32_t offset) {
    if (dynamicOffsets.size() <= set)
        dynamicOffsets.resize(set + 1);

    if (dynamicOffsets[set].size() <= binding)
        dynamicOffsets[set].resize(binding + 1);

    dynamicOffsets[set][binding] = offset;
}

std::tuple<VkDescriptorSet, uint32_t, uint32_t> Descriptor::FindDescriptorSet(const std::string &name) {
    auto indexOf = [](const std::vector<VkDescriptorSetLayout> &layouts, VkDescriptorSetLayout target) {
        for (uint32_t i = 0; i < layouts.size(); i++)
            if (layouts[i] == target)
                return i;
        return 0xffffffff;
    };

    VkDescriptorSet descriptorSet;

    auto it = pipelineLayout->mappings.find(name);
    if (it == pipelineLayout->mappings.end()) {
        throw std::runtime_error("Failed to find binding with name: " + name);
    }

    VkDescriptorSetLayout layout = it->second.first;
    uint32_t set = indexOf(pipelineLayout->descriptorSetLayouts, layout);
    uint32_t binding = it->second.second;

    // augment the descriptor sets array
    while (descriptorSets.size() <= set)
        descriptorSets.push_back(VK_NULL_HANDLE);

    if (descriptorSets[set] == VK_NULL_HANDLE)
        descriptorSets[set] = pool->Request(layout);

    while (dynamicOffsets.size() <= set)
        dynamicOffsets.push_back({ });

    descriptorSet = descriptorSets[set];
    return std::make_tuple(descriptorSet, set, binding);
}

//  ____                      _       _             ____             _
// |  _ \  ___  ___  ___ _ __(_)_ __ | |_ ___  _ __|  _ \ ___   ___ | |
// | | | |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| |_) / _ \ / _ \| |
// | |_| |  __/\__ \ (__| |  | | |_) | || (_) | |  |  __/ (_) | (_) | |
// |____/ \___||___/\___|_|  |_| .__/ \__\___/|_|  |_|   \___/ \___/|_|
//                             |_|

DescriptorPool::DescriptorPool(Device *device, uint32_t size) : device(device), maxPoolSets(size) {
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
        vkDestroyDescriptorPool(*device, pool, nullptr);
    }
}

void DescriptorPool::Reset() {
    // clearing allocated descriptors
    for (const auto &pool : pools) {
        ErrorCheck(vkResetDescriptorPool(*device, pool, (VkDescriptorPoolResetFlags) 0), "reset descriptor pool");
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
        ErrorCheck(vkCreateDescriptorPool(*device, &createInfo, nullptr, &pool), "create new descriptor pool");

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
    ErrorCheck(vkAllocateDescriptorSets(*device, &allocInfo, &descriptorSet), "allocate a descriptor set");

    // increment allocated set counter in the pool
    poolSetCounts[poolIndex]++;
    return descriptorSet;
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
    handle.stage = shader->GetInfo();
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

GraphicsPipelineDesc& GraphicsPipelineDesc::SetDepthTest(VkCompareOp compare) {
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
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
    desc.handle.stageCount = desc.shaderInfos.size();
    desc.handle.pStages = desc.shaderInfos.data();
    desc.handle.pNext = VK_NULL_HANDLE;
    desc.handle.layout = *layout;

    ErrorCheck(vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &desc.handle, nullptr, &handle), "create graphics pipeline");
}

Pipeline::Pipeline(Device *device, ComputePipelineDesc &desc) : device(device), bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE) {
    desc.Initialize(device);
    layout = desc.Layout();

    desc.handle.basePipelineHandle = handle;
    desc.handle.basePipelineIndex = 0;
    desc.handle.flags = 0;
    desc.handle.layout = *layout;

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
