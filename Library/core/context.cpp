#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/context.h"
#include "core/debug.h"

using namespace slim;

ContextDesc::ContextDesc() {
    // initialize physical device features
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = nullptr;
    features.features = {};
}

ContextDesc& ContextDesc::EnableValidation(bool value) {
    if (value) {
        PrepareForValidation();
    }
    validation = value;
    return *this;
}

ContextDesc& ContextDesc::EnableGraphics(bool value) {
    graphics = value;
    return *this;
}

ContextDesc& ContextDesc::EnableCompute(bool value) {
    compute = value;
    return *this;
}

ContextDesc& ContextDesc::EnableGLFW(bool value) {
    if (value) {
        PrepareForGlfw();
        PrepareForViewport();
        PrepareForSwapchain();
    }
    present = value;
    return *this;
}

ContextDesc& ContextDesc::EnableSeparateDepthStencilLayout() {
    // enable separate depth stencil layout
    deviceFeatures.separateDepthStencilLayout.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
    deviceFeatures.separateDepthStencilLayout.separateDepthStencilLayouts = VK_TRUE;
    // connect to features
    deviceFeatures.separateDepthStencilLayout.pNext = features.pNext;
    features.pNext = &deviceFeatures.separateDepthStencilLayout;
    return *this;
}

ContextDesc& ContextDesc::EnableDescriptorIndexing() {
    // add instance extensions
    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    // add device extensions
    deviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    // add physical device features
    deviceFeatures.descriptorIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    deviceFeatures.descriptorIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    deviceFeatures.descriptorIndexing.runtimeDescriptorArray = VK_TRUE;
    deviceFeatures.descriptorIndexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
    deviceFeatures.descriptorIndexing.descriptorBindingPartiallyBound = VK_TRUE;
    // connect to features
    deviceFeatures.descriptorIndexing.pNext = features.pNext;
    features.pNext = &deviceFeatures.descriptorIndexing;
    return *this;
}

void ContextDesc::PrepareForGlfw() {
    // query for glfw extensions
    glfwInit();
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        instanceExtensions.push_back(glfwExtensions[i]);
}

void ContextDesc::PrepareForViewport() {
    // MAINTENANCE1: contains the extension to allow negative height on viewport.
    // This would allow an OpenGL style viewport.
    deviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
}

void ContextDesc::PrepareForSwapchain() {
    // SWAPCHAIN: contains the extension for swapchain for presentation
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void ContextDesc::PrepareForValidation() {
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // TODO: need to query validation layer support
    const auto &supportedInstanceExtensions = GetSupportedInstanceExtensions();

    // list all instance layers
    if (validationLayers.size()) {
        std::cout << "[CreateInstance] with validation layers: " << std::endl;
        for (auto &layer : validationLayers)
            std::cout << "- " << layer << std::endl;
    }

    // list all instance extensions
    if (instanceExtensions.size()) {
        std::cout << "[CreateInstance] with instance extensions: " << std::endl;
        bool anyNotFound = false;
        for (auto &extension : instanceExtensions) {
            bool found = supportedInstanceExtensions.find(std::string(extension)) != supportedInstanceExtensions.end();
            std::cout << "- " << extension << ": ";
            std::cout << (found ? "FOUND" : "NOT FOUND") << std::endl;
            anyNotFound |= !found;
        }
        if (anyNotFound) {
            throw std::runtime_error("Failed to find all instance extensions!");
        }
    }
}

VkSurfaceKHR ContextDesc::PrepareSurface(VkInstance instance) const {
    // Vulkan is available, at least for compute
    if (!glfwVulkanSupported())
        throw std::runtime_error("GLFW does not support Vulkan!");

    // for Vulkan, use GLFW_NO_API
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // create a glfw window
    GLFWwindow *window = glfwCreateWindow(128, 128, "dummy", nullptr, nullptr);

    // This surface is a dummy one for picking physical device
    // create vulkan surface from glfw
    VkSurfaceKHR surface = VK_NULL_HANDLE;;
    ErrorCheck(glfwCreateWindowSurface(instance, window, nullptr, &surface), "create a dummy window surface");

    glfwDestroyWindow(window);

    return surface;
}

Context::Context(const ContextDesc& desc) : desc(desc) {
    InitInstance(desc);
    InitDebuggerMessener(desc);
    InitSurface(desc);
    InitPhysicalDevice(desc);
}

Context::~Context() {
    // clean up surface
    if (surface)  {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    // clean up debug messenger
    if (debugMessenger) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }

    // clean up vulkan instance
    if (instance) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}

void Context::InitInstance(const ContextDesc& desc) {
    // prepare application info
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Slim Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Slim Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // prepare instance info
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(desc.instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = desc.instanceExtensions.data();

    // prepare debug info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (desc.validation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(desc.validationLayers.size());
        createInfo.ppEnabledLayerNames = desc.validationLayers.data();
        FillVulkanDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // create vulkan instance
    ErrorCheck(vkCreateInstance(&createInfo, nullptr, &instance), "create instance");
}

void Context::InitDebuggerMessener(const ContextDesc& desc) {
    if (!desc.validation) return;

    // prepare the debug messenger
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    FillVulkanDebugMessengerCreateInfo(createInfo);

    // create the debug messenger
    ErrorCheck(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger), "setup debug messenger");
}

void Context::InitSurface(const ContextDesc& desc) {
    surface = desc.PrepareSurface(instance);
}

void Context::InitPhysicalDevice(const ContextDesc &desc) {
    // define a function to pick preferred physical device
    auto rateDevice = [desc](VkPhysicalDevice device, VkSurfaceKHR surface) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        int score = 0;

        // maximum possible size of textures affetcs graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        // prefer discrete GPU because it usually have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;

        // rule out this device if it does not match our requirement
        QueueFamilyIndices indices = FindQueueFamilyIndices(device, surface);
        if (desc.compute && !indices.compute.has_value()) score *= 0;
        if (desc.graphics && !indices.graphics.has_value()) score *= 0;
        if (desc.present && !indices.present.has_value()) score *= 0;

        // rule out this device if swapchain support is not adequate
        if (desc.present) {
            SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device, surface);
            if (swapchainSupport.formats.empty() && swapchainSupport.presentModes.empty()) {
                score *= 0;
            }
        }

        return score;
    };

    // find number of physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    // get all physical device objects
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // store scores of all devices
    std::vector<std::pair<VkPhysicalDevice,int>> candidates;
    for (const auto& device : devices) {
        bool adequate = CheckDeviceExtensionSupport(device, desc.deviceExtensions);
        if (adequate) {
            int score = rateDevice(device, surface);
            candidates.push_back(std::make_pair(device, score));
        }
    }

    // pick the device with highest score
    int score = 0;
    for (const auto &kv : candidates) {
        if (kv.second > score) {
            score = kv.second;
            physicalDevice = kv.first;
        }
    }

    // sanity check
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}
