#include "core/sampler.h"
#include "core/debug.h"
#include "core/vkutils.h"

using namespace slim;

SamplerDesc::SamplerDesc() {
    handle = {};
    handle.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    handle.flags = 0;
    handle.magFilter = VK_FILTER_LINEAR;
    handle.minFilter = VK_FILTER_LINEAR;
    handle.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    handle.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    handle.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    handle.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    handle.mipLodBias = 0.0f;
    handle.anisotropyEnable = false;
    handle.maxAnisotropy = 0.0f;
    handle.compareEnable = false;
    handle.compareOp = VK_COMPARE_OP_ALWAYS;
    handle.minLod = 0.0f;
    handle.maxLod = 1.0f;
    handle.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    handle.unnormalizedCoordinates = false;
}

SamplerDesc::~SamplerDesc() {

}

SamplerDesc& SamplerDesc::UseUnnormalizedCoordinates(bool value) {
    handle.unnormalizedCoordinates = value;
    return *this;
}

SamplerDesc& SamplerDesc::MagFilter(VkFilter filter) {
    handle.magFilter = filter;
    return *this;
}

SamplerDesc& SamplerDesc::MinFilter(VkFilter filter) {
    handle.minFilter = filter;
    return *this;
}

SamplerDesc& SamplerDesc::MipmapMode(VkSamplerMipmapMode mode) {
    handle.mipmapMode = mode;
    return *this;
}

SamplerDesc& SamplerDesc::MaxAnistropy(float value) {
    handle.anisotropyEnable = true;
    handle.maxAnisotropy = value;
    return *this;
}

SamplerDesc& SamplerDesc::SetCompareOp(VkCompareOp op) {
    handle.compareEnable = true;
    handle.compareOp = op;
    return *this;
}

SamplerDesc& SamplerDesc::LOD(float minLod, float maxLod) {
    handle.minLod = minLod;
    handle.maxLod = maxLod;
    return *this;
}

SamplerDesc& SamplerDesc::AddressMode(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w) {
    handle.addressModeU = u;
    handle.addressModeV = v;
    handle.addressModeW = w;
    return *this;
}

Sampler::Sampler(Device *device, const SamplerDesc &desc) : device(device) {
    ErrorCheck(DeviceDispatch(vkCreateSampler(*device, &desc.handle, nullptr, &handle)), "create sampler");
}

Sampler::~Sampler() {
    if (handle) {
        DeviceDispatch(vkDestroySampler(*device, handle, nullptr));
        handle = VK_NULL_HANDLE;
    }
}

void Sampler::SetName(const std::string& name) const {
    if (device->IsDebuggerEnabled()) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT;
        nameInfo.object = (uint64_t) handle;
        nameInfo.pObjectName = name.c_str();
        ErrorCheck(DeviceDispatch(vkDebugMarkerSetObjectNameEXT(*device, &nameInfo)), "set sampler name");
    }
}
