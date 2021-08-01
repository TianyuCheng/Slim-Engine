#ifndef SLIM_CORE_SAMPLER_H
#define SLIM_CORE_SAMPLER_H

#include <list>
#include <string>
#include <vector>
#include <VmaUsage.h>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "core/hasher.h"
#include "core/device.h"
#include "utility/interface.h"

namespace slim {

    class SamplerDesc final : public TriviallyConvertible<VkSamplerCreateInfo> {
        friend class Sampler;
    public:
        explicit SamplerDesc();
        virtual ~SamplerDesc();

        SamplerDesc& UseUnnormalizedCoordinates(bool value = true);

        SamplerDesc& MagFilter(VkFilter filter);

        SamplerDesc& MinFilter(VkFilter filter);

        SamplerDesc& MipmapMode(VkSamplerMipmapMode mode);

        SamplerDesc& MaxAnistropy(float value);

        SamplerDesc& SetCompareOp(VkCompareOp op);

        SamplerDesc& LOD(float minLod, float maxLod);

        SamplerDesc& AddressMode(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    };

    class Sampler final : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkSampler> {
    public:
        explicit Sampler(Device *device, const SamplerDesc &desc);
        virtual ~Sampler();
    private:
        Device* device = nullptr;
    };

} // end of namespace slim

#endif // end of SLIM_CORE_SAMPLER_H
