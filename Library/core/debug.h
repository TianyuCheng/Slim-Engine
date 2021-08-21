#ifndef SLIM_CORE_DEBUG_H
#define SLIM_CORE_DEBUG_H

#include <cassert>

#ifdef SLIM_VULKAN_SUPPORT
#include <vulkan/vulkan.h>
#include "core/vkutils.h"

#ifdef NDEBUG
#define ErrorCheck(result, message) { result }
#else
#define ErrorCheck(result, message)                                                                                            \
    if ((result) != VK_SUCCESS) {                                                                                              \
        std::cerr << "[VkError][" << message << "] " << __FILE__ << ":" << __LINE__ << " | " << ToString(result) << std::endl; \
        assert(!!!"There was an error in Vk function call!");                                                                  \
    }
#endif

#endif // SLIM_CORE_SUPPORT
#endif // SLIM_CORE_DEBUG_H
