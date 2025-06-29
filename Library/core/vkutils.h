#ifndef SLIM_CORE_VKUTILS_H
#define SLIM_CORE_VKUTILS_H

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#endif

#include <vector>
#include <iostream>
#include <optional>
#include <unordered_set>

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/vulkan.h"

namespace slim {

    class Image;
    class CommandBuffer;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> compute;
        std::optional<uint32_t> present;
        std::optional<uint32_t> transfer;
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const uint32_t VKAPIVersion = VK_API_VERSION_1_2;

    std::unordered_set<std::string> GetSupportedInstanceExtensions();

    std::unordered_set<std::string> CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> &deviceExtensions);

    QueueFamilyIndices FindQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface = VK_NULL_HANDLE);

    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    VkSampleCountFlags GetMaxUsableSampleCount(VkPhysicalDevice device);

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D ChooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR& capabilities);

    void LayoutTransition(CommandBuffer* cmdbuffer,
                          Image *image,
                          const VkImageSubresourceRange &subresources,
                          VkImageLayout srcLayout,
                          VkImageLayout dstLayout,
                          VkAccessFlags srcAccessMask,
                          VkAccessFlags dstAccessMask,
                          VkPipelineStageFlags srcStageMask,
                          VkPipelineStageFlags dstStageMask);

    void PrepareLayoutTransition(CommandBuffer* cmdbuffer,
                                 Image *image,
                                 VkImageLayout srcLayout,
                                 VkImageLayout dstLayout,
                                 VkPipelineStageFlags srcStageMask,
                                 VkPipelineStageFlags dstStageMask,
                                 uint32_t baseLayer, uint32_t layerCount,
                                 uint32_t mipLevel, uint32_t mipCount);

    void FillVulkanDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &info);

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                          const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator);

    std::string ToString(VkResult result);

} // end of slim namespace

#endif // SLIM_CORE_VKUTILS_H
