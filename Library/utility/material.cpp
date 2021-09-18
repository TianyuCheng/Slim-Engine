#include "utility/material.h"

using namespace slim;
using namespace slim::scene;

Material::Material(Device *device) : device(device), technique(nullptr) {
    // a material should not need too many descriptors,
    // 32 should be large enough for most cases without
    // needing to create additional descriptorPool
    descriptorPool = SlimPtr<DescriptorPool>(device, 32);
    uniformBufferPool = SlimPtr<BufferPool<UniformBuffer>>(device);
}

Material::Material(Device *device, Technique *technique) : device(device), technique(technique) {
    // a material should not need too many descriptors,
    // 32 should be large enough for most cases without
    // needing to create additional descriptorPool
    descriptorPool = SlimPtr<DescriptorPool>(device, 32);
    uniformBufferPool = SlimPtr<BufferPool<UniformBuffer>>(device);

    SetTechnique(technique);
}

Material::~Material() {

}

void Material::SetTechnique(Technique* technique) {
    if (technique == nullptr) return;

    // need to initialize layout
    for (Technique::Pass &pass : *technique) {
        pass.desc.Initialize(device);
    }

    // initialize descriptors
    for (Technique::Pass &pass : *technique) {
        descriptors.push_back(SlimPtr<Descriptor>(descriptorPool, pass.desc.Layout()));
    }

    this->technique = technique;
}

void Material::SetTexture(const std::string &name, Image *texture, Sampler *sampler) {
    #ifndef NDEBUG
    bool found = false;
    #endif
    for (Descriptor* descriptor : descriptors) {
        if (descriptor->HasBinding(name)) {
            descriptor->SetTexture(name, texture, sampler);
            #ifndef NDEBUG
            found = true;
            #endif
        }
    }
    #ifndef NDEBUG
    if (!found) {
        throw std::runtime_error("[Material::SetTexture] no binding named '" + name + "'");
    }
    #endif
}

void Material::SetUniformBuffer(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    #ifndef NDEBUG
    bool found = false;
    #endif
    for (Descriptor* descriptor : descriptors) {
        if (descriptor->HasBinding(name)) {
            descriptor->SetUniformBuffer(name, BufferAlloc { buffer, offset ,size });
            #ifndef NDEBUG
            found = true;
            #endif
        }
    }
    #ifndef NDEBUG
    if (!found) {
        throw std::runtime_error("[Material::SetUniform] no binding named '" + name + "'");
    }
    #endif
}

void Material::SetStorageBuffer(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    #ifndef NDEBUG
    bool found = false;
    #endif
    for (Descriptor* descriptor : descriptors) {
        if (descriptor->HasBinding(name)) {
            descriptor->SetStorageBuffer(name, BufferAlloc { buffer, offset ,size });
            #ifndef NDEBUG
            found = true;
            #endif
        }
    }
    #ifndef NDEBUG
    if (!found) {
        throw std::runtime_error("[Material::SetStorage] no binding named '" + name + "'");
    }
    #endif
}

void Material::Bind(uint32_t index,
                    CommandBuffer *commandBuffer,
                    RenderFrame *renderFrame,
                    RenderPass *renderPass) const {

    // bind pipeline used by this technique + render queue
    technique->Bind(index, renderFrame, renderPass, commandBuffer);

    // bind descriptor
    VkPipelineBindPoint bindPoint = technique->Type(index);
    commandBuffer->BindDescriptor(descriptors[index], bindPoint);
}

bool Material::HasID() const {
    return materialId >= 0;
}

uint32_t Material::GetID() const {
    #ifndef NDEBUG
    if (materialId < 0) {
        throw std::runtime_error("[Material::GetID] This material does not have a valid material id.");
    }
    #endif
    return static_cast<uint32_t>(materialId);
}

void Material::SetID(uint32_t id) {
    materialId = static_cast<int32_t>(id);
    #ifndef NDEBUG
    if (materialId < 0) {
        throw std::runtime_error("[Material::SetID] This material is assigned an invalid material id.");
    }
    #endif
}

void Material::ResetID() {
    materialId = -1;
}
