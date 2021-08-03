#include <algorithm>
#include "core/debug.h"
#include "core/vkutils.h"
#include "core/commands.h"
#include "core/descriptor.h"
#include "core/renderframe.h"

using namespace slim;

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
    auto it = std::max_element(writeDescriptorSets.begin(), writeDescriptorSets.end());

    // clear descriptor sets
    descriptorSets.resize(*it + 1);
    std::fill(descriptorSets.begin(), descriptorSets.end(), VK_NULL_HANDLE);

    // allocate descriptor set if needed to
    for (uint32_t i = 0; i < writeDescriptorSets.size(); i++) {
        uint32_t set = writeDescriptorSets[i];
        if (descriptorSets[set] == VK_NULL_HANDLE) {
            uint32_t variableDescriptorCount = variableDescriptorCounts[set];
            descriptorSets[set] = pool->Request(pipelineLayout->GetSetBindings(set).layout, variableDescriptorCount);
        }
    }

    // assign dst set
    for (uint32_t i = 0; i < writes.size(); i++) {
        VkWriteDescriptorSet& write = writes[i];
        uint32_t set = writeDescriptorSets[i];
        write.dstSet = descriptorSets[set];
    }

    // update descriptor sets
    vkUpdateDescriptorSets(*pool->GetDevice(), writes.size(), writes.data(), 0, nullptr);

    // clear existing updates
    writes.clear();
    imageInfos.clear();
    bufferInfos.clear();
    writeDescriptorSets.clear();
    variableDescriptorCounts.clear();
}

bool Descriptor::HasBinding(const std::string &name) const {
    return pipelineLayout->HasBinding(name);
}

std::tuple<uint32_t, uint32_t> Descriptor::GetBinding(const std::string &name) {
    auto [set, binding, _] = FindDescriptorSet(name);
    return std::make_tuple(set, binding);
}

void Descriptor::SetTexture(const std::string &name, Image *image, Sampler *sampler) {
    SetTextures(name, { image }, { sampler });
}

void Descriptor::SetTextures(const std::string &name, const std::vector<Image*> &images, const std::vector<Sampler*> &samplers) {
    auto [set, binding, flags] = FindDescriptorSet(name);

    #ifdef NDEBUG
    if (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && imageNum != samplerNum) {
        throw std::runtime_error("The number of images and samplers must match for combined image+sampler");
    }
    #endif

    imageInfos.push_back(std::vector<VkDescriptorImageInfo> { });
    auto& infos = imageInfos.back();
    infos.reserve(images.size());

    for (uint32_t i = 0; i < images.size(); i++) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = images[i]->AsTexture();
        imageInfo.sampler = *samplers[i];
        infos.push_back(imageInfo);
    }

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    update.dstSet = VK_NULL_HANDLE;
    update.dstBinding = binding;
    update.dstArrayElement = 0;
    update.descriptorCount = infos.size();
    update.pImageInfo = infos.data();
    update.pBufferInfo = nullptr;
    update.pTexelBufferView = nullptr;

    writes.push_back(update);
    writeDescriptorSets.push_back(set);

    if (flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)
        variableDescriptorCounts[set] = images.size();
}

void Descriptor::SetImage(const std::string &name, Image *image) {
    SetImages(name, { image });
}

void Descriptor::SetImages(const std::string &name, const std::vector<Image*> &images) {
    auto [set, binding, flags] = FindDescriptorSet(name);

    imageInfos.push_back(std::vector<VkDescriptorImageInfo> { });
    auto& infos = imageInfos.back();
    infos.reserve(images.size());

    for (auto image : images) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = image->AsTexture();
        imageInfo.sampler = nullptr;
        infos.push_back(imageInfo);
    }

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    update.dstSet = VK_NULL_HANDLE;
    update.dstBinding = binding;
    update.dstArrayElement = 0;
    update.descriptorCount = infos.size();
    update.pImageInfo = infos.data();
    update.pBufferInfo = nullptr;
    update.pTexelBufferView = nullptr;

    writes.push_back(update);
    writeDescriptorSets.push_back(set);

    if (flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)
        variableDescriptorCounts[set] = images.size();
}

void Descriptor::SetSampler(const std::string &name, Sampler *sampler) {
    SetSamplers(name, { sampler });
}

void Descriptor::SetSamplers(const std::string &name, const std::vector<Sampler*> &samplers) {
    auto [set, binding, flags] = FindDescriptorSet(name);

    imageInfos.push_back(std::vector<VkDescriptorImageInfo> { });
    auto& infos = imageInfos.back();
    infos.reserve(samplers.size());

    for (auto sampler : samplers) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = *sampler;
        infos.push_back(imageInfo);
    }

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    update.dstSet = VK_NULL_HANDLE;
    update.dstBinding = binding;
    update.dstArrayElement = 0;
    update.descriptorCount = infos.size();
    update.pImageInfo = infos.data();
    update.pBufferInfo = nullptr;
    update.pTexelBufferView = nullptr;

    writes.push_back(update);
    writeDescriptorSets.push_back(set);

    if (flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)
        variableDescriptorCounts[set] = samplers.size();
}

void Descriptor::SetUniform(const std::string &name, Buffer* buffer) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { BufferAlloc(buffer) });
}

void Descriptor::SetUniform(const std::string &name, const BufferAlloc& alloc) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { alloc });
}

void Descriptor::SetUniforms(const std::string &name, const std::vector<BufferAlloc> &allocs) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, allocs);
}

void Descriptor::SetDynamic(const std::string &name, Buffer *buffer, size_t elemSize) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, { BufferAlloc(buffer, 0, elemSize) });
}

void Descriptor::SetDynamic(const std::string &name, const BufferAlloc& alloc) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, { alloc });
}

void Descriptor::SetStorage(const std::string &name, Buffer* buffer) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, { BufferAlloc(buffer) });
}

void Descriptor::SetStorage(const std::string &name, const BufferAlloc& alloc) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, { alloc });
}

void Descriptor::SetStorages(const std::string &name, const std::vector<BufferAlloc>& allocs) {
    SetBuffer(name, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, allocs);
}

void Descriptor::SetBuffer(const std::string &name, VkDescriptorType descriptorType, const std::vector<BufferAlloc> &bufferAllocs) {
    auto [set, binding, flags] = FindDescriptorSet(name);

    bufferInfos.push_back(std::vector<VkDescriptorBufferInfo> { });
    auto& infos = bufferInfos.back();
    infos.reserve(bufferAllocs.size());

    // flush buffer
    for (const auto& alloc : bufferAllocs) {
        alloc.buffer->Flush();
        VkDescriptorBufferInfo info = {};
        info.buffer = *alloc.buffer;
        info.offset = alloc.offset;
        info.range = alloc.size == 0 ? alloc.buffer->Size() : alloc.size;
        infos.push_back(info);
    }

    VkWriteDescriptorSet update = {};
    update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    update.pNext = nullptr;
    update.descriptorType = descriptorType;
    update.dstSet = VK_NULL_HANDLE;
    update.dstBinding = binding;
    update.dstArrayElement = 0;
    update.descriptorCount = infos.size();
    update.pImageInfo = nullptr;
    update.pBufferInfo = infos.data();
    update.pTexelBufferView = nullptr;

    writes.push_back(update);
    writeDescriptorSets.push_back(set);

    if (flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)
        variableDescriptorCounts[set] = bufferAllocs.size();
}

void Descriptor::SetDynamicOffset(const std::string &name, uint32_t offset) {
    auto [set, binding, _] = FindDescriptorSet(name);
    SetDynamicOffset(set, binding, offset);
}

void Descriptor::SetDynamicOffset(uint32_t set, uint32_t binding, uint32_t offset) {
    if (dynamicOffsets.size() <= set)
        dynamicOffsets.resize(set + 1);

    if (dynamicOffsets[set].size() <= binding)
        dynamicOffsets[set].resize(binding + 1);

    dynamicOffsets[set][binding] = offset;
}

std::tuple<uint32_t, uint32_t, VkDescriptorBindingFlags> Descriptor::FindDescriptorSet(const std::string &name) {
    uint32_t indexAccessor = 0;
    const DescriptorSetLayout& layout = pipelineLayout->GetSetBinding(name, indexAccessor);

    uint32_t set = layout.bindings[indexAccessor].set;
    uint32_t binding = layout.bindings[indexAccessor].binding;
    VkDescriptorBindingFlags flags = layout.bindings[indexAccessor].bindingFlags;

    if (dynamicOffsets.size() <= set)
        dynamicOffsets.resize(set + 1);

    return std::make_tuple(set, binding, flags);
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

VkDescriptorSet DescriptorPool::Request(VkDescriptorSetLayout layout, uint32_t variableDescriptorCount) {
    poolIndex = FindAvailablePoolIndex(poolIndex);

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = pools[poolIndex];
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layout;

    VkDescriptorSetVariableDescriptorCountAllocateInfo setCounts;
    if (variableDescriptorCount) {
        setCounts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
        setCounts.descriptorSetCount = 1;
        setCounts.pDescriptorCounts = &variableDescriptorCount;
        setCounts.pNext = nullptr;
        allocInfo.pNext = &setCounts;
    }

    ErrorCheck(vkAllocateDescriptorSets(*device, &allocInfo, &descriptorSet), "allocate a descriptor set");

    // increment allocated set counter in the pool
    poolSetCounts[poolIndex]++;
    return descriptorSet;
}

