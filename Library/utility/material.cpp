#include "utility/material.h"

using namespace slim;

Material::Material(Device *device, Technique *technique) : technique(technique) {
    // a material should not need too many descriptors,
    // 32 should be large enough for most cases without
    // needing to create additional descriptorPool
    descriptorPool = SlimPtr<DescriptorPool>(device, 32);
    uniformBufferPool = SlimPtr<BufferPool<UniformBuffer>>(device);

    // need to initialize layout
    for (Technique::Pass &pass : *technique) {
        pass.desc.Initialize(device);
    }

    // initialize descriptors
    for (Technique::Pass &pass : *technique) {
        descriptors.push_back(SlimPtr<Descriptor>(descriptorPool, pass.desc.Layout()));
    }
}

Material::~Material() {

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

void Material::SetUniform(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    #ifndef NDEBUG
    bool found = false;
    #endif
    for (Descriptor* descriptor : descriptors) {
        if (descriptor->HasBinding(name)) {
            descriptor->SetUniform(name, BufferAlloc { buffer, offset ,size });
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

void Material::SetStorage(const std::string &name, Buffer *buffer, size_t offset, size_t size) {
    #ifndef NDEBUG
    bool found = false;
    #endif
    for (Descriptor* descriptor : descriptors) {
        if (descriptor->HasBinding(name)) {
            descriptor->SetStorage(name, BufferAlloc { buffer, offset ,size });
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
