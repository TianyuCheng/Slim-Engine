#ifndef SLIM_UTILITY_TEXTURE_H
#define SLIM_UTILITY_TEXTURE_H

#include <list>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>

#include "core/vulkan.h"
#include "core/image.h"
#include "core/context.h"
#include "core/commands.h"
#include "utility/stb.h"

namespace slim {

    class TextureLoader {
    public:
        static void FlipVerticallyOnLoad(bool value = true);

        static GPUImage* Load2D(CommandBuffer* commandBuffer,
                                const std::string& filename,
                                VkFilter filter = VK_FILTER_LINEAR);

        static GPUImage* LoadCubemap(CommandBuffer* commandBuffer,
                                     const std::string& xpos,
                                     const std::string& xneg,
                                     const std::string& ypos,
                                     const std::string& yneg,
                                     const std::string& zpos,
                                     const std::string& zneg,
                                     VkFilter filter = VK_FILTER_LINEAR);

    private:
        static GPUImage* Load2DLDR(CommandBuffer* commandBuffer, uint8_t* data, uint32_t width, uint32_t height, uint32_t numChannels, VkFilter filter);
        static GPUImage* Load2DHDR(CommandBuffer*commandBuffer, float* data, uint32_t width, uint32_t height, uint32_t numChannels, VkFilter filter);

        static GPUImage* LoadCubemapLDR(CommandBuffer* commandBuffer,
                                        const std::string& xpos,
                                        const std::string& xneg,
                                        const std::string& ypos,
                                        const std::string& yneg,
                                        const std::string& zpos,
                                        const std::string& zneg,
                                        VkFilter filter = VK_FILTER_LINEAR);
        static GPUImage* LoadCubemapHDR(CommandBuffer* commandBuffer,
                                        const std::string& xpos,
                                        const std::string& xneg,
                                        const std::string& ypos,
                                        const std::string& yneg,
                                        const std::string& zpos,
                                        const std::string& zneg,
                                        VkFilter filter = VK_FILTER_LINEAR);
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_TEXTURE_H
