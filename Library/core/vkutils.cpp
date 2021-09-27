#include <sstream>
#include <iostream>
#include <algorithm>
#include "core/vkutils.h"
#include "core/image.h"
#include "core/renderpass.h"
#include "utility/color.h"

namespace slim {

    std::string ToString(VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
        switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:        return "Verbose";
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:           return "Info";
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:        return "Warning";
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:          return "Error";
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: return "MaxEnum";
        }
        return "Unknown";
    }

    std::string ToString(VkDebugUtilsMessageTypeFlagsEXT type) {
        switch (type) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:        return "General";
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:     return "Validation";
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:    return "Performance";
            case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT: return "MaxEnum";
        }
        return "Unknown";
    }

    std::string ToString(VkResult result) {
        switch (result) {
            case VK_SUCCESS:
                return "[VkResult] Success";
            case VK_NOT_READY:
                return "[VkResult] A fence or query has not yet completed";
            case VK_TIMEOUT:
                return "[VkResult] A wait operation has not completed in the specified time";
            case VK_EVENT_SET:
                return "[VkResult] An event is signaled";
            case VK_EVENT_RESET:
                return "[VkResult] An event is unsignaled";
            case VK_INCOMPLETE:
                return "[VkResult] A return array was too small for the result";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "[VkResult] A host memory allocation has failed";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "[VkResult] A device memory allocation has failed";
            case VK_ERROR_INITIALIZATION_FAILED:
                return "[VkResult] Initialization of an object could not be completed for implementation-specific reasons";
            case VK_ERROR_DEVICE_LOST:
                return "[VkResult] The logical or physical device has been lost";
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "[VkResult] Mapping of a memory object has failed";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "[VkResult] A requested layer is not present or could not be loaded";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "[VkResult] A requested extension is not supported";
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "[VkResult] A requested feature is not supported";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "[VkResult] The requested version of Vulkan is not supported by the driver or is otherwise incompatible";
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "[VkResult] Too many objects of the type have already been created";
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "[VkResult] A requested format is not supported on this device";
            case VK_ERROR_SURFACE_LOST_KHR:
                return "[VkResult] A surface is no longer available";
            case VK_SUBOPTIMAL_KHR:
                return "[VkResult] A swapchain no longer matches the surface properties exactly, but can still be used";
            case VK_ERROR_OUT_OF_DATE_KHR:
                return "[VkResult] A surface has changed in such a way that it is no longer compatible with the swapchain";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                return "[VkResult] The display used by a swapchain does not use the same presentable image layout";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return "[VkResult] The requested window is already connected to a VkSurfaceKHR, or to some other non-Vulkan API";
            case VK_ERROR_VALIDATION_FAILED_EXT:
                return "[VkResult] A validation layer found an error";
            default:
                return "[VkResult] ERROR: UNKNOWN VULKAN ERROR";
        }
        return "unknown";
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData) {
        std::cerr << "[Vulkan]";

        // message type
        switch (messageType) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                std::cerr << slim::color::Modifier(slim::color::FG_BLUE);
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                std::cerr << slim::color::Modifier(slim::color::FG_YELLOW);
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                std::cerr << slim::color::Modifier(slim::color::FG_YELLOW);
                break;
            default:
                break;
        }
        std::cerr << "[" << ToString(messageType) << "]";
        std::cerr << slim::color::Modifier(slim::color::FG_DEFAULT);

        // message severity
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                std::cerr << slim::color::Modifier(slim::color::FG_BLUE);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                std::cerr << slim::color::Modifier(slim::color::FG_BLUE);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                std::cerr << slim::color::Modifier(slim::color::FG_YELLOW);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                std::cerr << slim::color::Modifier(slim::color::FG_RED);
                break;
            default:
                break;
        }
        std::cerr << "[" << ToString(messageSeverity) << "]";
        std::cerr << slim::color::Modifier(slim::color::FG_DEFAULT);

        // message
        std::cerr << " " << pCallbackData->pMessage << std::endl;

        (void) pUserData;
        (void) pCallbackData;
        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void FillVulkanDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &info) {
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = slim::DebugCallback;
        info.pNext = nullptr;
        info.flags = 0;
    }

    std::unordered_set<std::string> CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> &deviceExtensions) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions;
    }

    QueueFamilyIndices FindQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        QueueFamilyIndices indices;

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // check graphics queue family
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphics = i;

            // check compute queue family
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
                indices.compute = i;

            // check transfer queue family
            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                indices.transfer = i;

            // check present queue family
            if (surface != VK_NULL_HANDLE) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport) indices.present = i;
            }

            if (surface != VK_NULL_HANDLE) {
                // check for all queue families
                if (indices.graphics.has_value() &&
                    indices.compute.has_value() &&
                    indices.present.has_value()) {
                    break;
                }
            } else {
                // check for only compute and graphics families
                if (indices.graphics.has_value() &&
                    indices.compute.has_value()) {
                    break;
                }
            }

            i++;
        }

        return indices;
    }

    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSampleCountFlags GetMaxUsableSampleCount(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts
                                  & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT;  }
        if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT;  }
        if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT;  }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    std::unordered_set<std::string> GetSupportedInstanceExtensions() {
        uint32_t count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
        std::unordered_set<std::string> results;
        for (auto &extension : extensions)
            results.insert(extension.extensionName);
        return results;
    }

    void LayoutTransition(VkCommandBuffer cmdbuffer,
                          Image *image,
                          const VkImageSubresourceRange &subresources,
                          VkImageLayout srcLayout,
                          VkImageLayout dstLayout,
                          VkAccessFlags srcAccessMask,
                          VkAccessFlags dstAccessMask,
                          VkPipelineStageFlags srcStageMask,
                          VkPipelineStageFlags dstStageMask) {
        // no need to transition
        if (srcLayout == dstLayout) return;

        VkImageMemoryBarrier barrier            = {};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = srcLayout;
        barrier.newLayout                       = dstLayout;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = *image;
        barrier.subresourceRange.aspectMask     = subresources.aspectMask;
        barrier.subresourceRange.baseMipLevel   = subresources.baseMipLevel;
        barrier.subresourceRange.levelCount     = subresources.levelCount;
        barrier.subresourceRange.baseArrayLayer = subresources.baseArrayLayer;
        barrier.subresourceRange.layerCount     = subresources.layerCount;
        barrier.srcAccessMask                   = srcAccessMask;
        barrier.dstAccessMask                   = dstAccessMask;

        vkCmdPipelineBarrier(
            cmdbuffer,                          // cmdbuffer
            srcStageMask, dstStageMask,         // srcStageMask, dstStageMask
            0,                                  // dependency flags
            0, nullptr,                         // memoryBarrierCount, pMemoryBarriers
            0, nullptr,                         // bufferMemoryBarrierCount, pBufferMemoryBarriers,
            1, &barrier                         // imageMemoryBarrierCount, pImageMemoryBarriers
        );
    }

    void PrepareLayoutTransition(VkCommandBuffer cmdbuffer,
                                 Image *image,
                                 VkImageLayout srcLayout,
                                 VkImageLayout dstLayout,
                                 VkPipelineStageFlags srcStageMask,
                                 VkPipelineStageFlags dstStageMask,
                                 uint32_t baseLayer, uint32_t layerCount,
                                 uint32_t mipLevel, uint32_t mipCount) {
        // no need to transition
        if (srcLayout == dstLayout) return;

        VkAccessFlags srcAccessMask = 0;
        VkAccessFlags dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel   = mipLevel;
        subresourceRange.levelCount     = mipCount;
        subresourceRange.baseArrayLayer = baseLayer;
        subresourceRange.layerCount     = layerCount;

        switch (srcLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                srcAccessMask = 0;
                break;
            case VK_IMAGE_LAYOUT_GENERAL:
                srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                break;
            // combined depth stencil
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
                srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                if (!IsDepthOnly(image->GetFormat())) {
                    subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                break;
            // separate depth only
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            // separate stencil only
            case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
                srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                break;
            default:
                std::cerr << "src layout: " << srcLayout << std::endl;
                throw std::runtime_error("unimplemented image layout transition");
        }

        switch (dstLayout) {
            case VK_IMAGE_LAYOUT_GENERAL:
                dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                break;
            default:
                break;
        }

        LayoutTransition(cmdbuffer, image,
                         subresourceRange,
                         srcLayout, dstLayout,
                         srcAccessMask, dstAccessMask,
                         srcStageMask, dstStageMask);

        for (uint32_t i = baseLayer; i < baseLayer + layerCount; i++)
            for (uint32_t j = mipLevel; j < mipLevel + mipCount; j++)
                image->layouts[i][j] = dstLayout;
    }

} // end of slim namespace
