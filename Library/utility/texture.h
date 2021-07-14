#ifndef SLIM_UTILITY_TEXTURE_H
#define SLIM_UTILITY_TEXTURE_H

#include <list>
#include <string>
#include <vector>
#include <VmaUsage.h>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "core/image.h"
#include "core/context.h"
#include "core/commands.h"
#include "utility/stb.h"

namespace slim {

    class TextureLoader {
    public:
        static GPUImage2D* Load2D(CommandBuffer *commandBuffer,
                                  const std::string &filename, VkFilter filter = VK_FILTER_LINEAR);
    private:
        static GPUImage2D* Load2DLDR(CommandBuffer *commandBuffer,
                                     uint8_t *data, uint32_t width, uint32_t height,
                                     uint32_t numChannels, VkFilter filter);
        static GPUImage2D* Load2DHDR(CommandBuffer *commandBuffer,
                                     float *data, uint32_t width, uint32_t height,
                                     uint32_t numChannels, VkFilter filter);
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_TEXTURE_H
