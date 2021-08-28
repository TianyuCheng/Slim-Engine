#ifndef SLIM_CORE_LOADER_H
#define SLIM_CORE_LOADER_H

#include <iostream>

#define VK_NO_PROTOTYPES
#include <volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <VmaUsage.h>

namespace slim {

    inline void Initialize() {
        // initialize volk
        if (volkInitialize() != VK_SUCCESS) {
            std::cerr << "[Vulkan|Volk] Initialization Failed!" << std::endl;
        }
    }

}

#endif // SLIM_CORE_LOADER_H
