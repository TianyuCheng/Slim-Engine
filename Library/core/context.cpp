#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/debug.h"
#include "core/window.h"
#include "core/context.h"
#include "core/renderframe.h"

using namespace slim;

ContextDesc& ContextDesc::EnableValidation(bool value) {
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

Context::Context(const ContextDesc &contextDesc) {
    // prepare extensions and layers
    PrepareForViewport();
    if (contextDesc.validation)
        PrepareForValidation();

    // initialization
    InitInstance(contextDesc);
    InitDebuggerMessener(contextDesc);
    InitPhysicalDevice(contextDesc);
    InitLogicalDevice(contextDesc);
    InitMemoryAllocator(contextDesc);
}

Context::Context(const ContextDesc &contextDesc, const WindowDesc &windowDesc) {
    contextDesc.present = true;
    glfwInit();

    // prepare extensions and layers
    PrepareForWindow();
    PrepareForSwapchain();
    PrepareForViewport();
    if (contextDesc.validation)
        PrepareForValidation();

    // initialization
    InitInstance(contextDesc);
    InitDebuggerMessener(contextDesc);
    InitSurface(contextDesc);
    InitPhysicalDevice(contextDesc);
    InitLogicalDevice(contextDesc);
    InitMemoryAllocator(contextDesc);

    // create a window for swapchain supports
    window = new Window(this, windowDesc);
}

Context::~Context() {
    WaitIdle();

    // clean up glfw window system
    if (window) {
        delete window;
        window = nullptr;
        glfwTerminate();
    }

    // clean up memory allocator
    if (allocator) {
        vmaDestroyAllocator(allocator);
        allocator = VK_NULL_HANDLE;
    }

    // clean up logical device
    if (device) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

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

void Context::PrepareForWindow() {
    // query for glfw extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        instanceExtensions.push_back(glfwExtensions[i]);
}

void Context::PrepareForSwapchain() {
    // SWAPCHAIN: contains the extension for swapchain for presentation
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Context::PrepareForViewport() {
    // MAINTENANCE1: contains the extension to allow negative height on viewport.
    // This would allow an OpenGL style viewport.
    deviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
}

void Context::PrepareForValidation() {
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

void Context::InitInstance(const ContextDesc &desc) {
    // prepare application info
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Slim Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Slim Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // prepare instance info
    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    // prepare debug info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (desc.validation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        FillVulkanDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // create vulkan instance
    ErrorCheck(vkCreateInstance(&createInfo, nullptr, &instance), "create instance");
}

void Context::InitDebuggerMessener(const ContextDesc &desc) {
    if (!desc.validation) return;

    // prepare the debug messenger
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    FillVulkanDebugMessengerCreateInfo(createInfo);

    // create the debug messenger
    ErrorCheck(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger), "setup debug messenger");
}

void Context::InitSurface(const ContextDesc &desc) {
    (void) desc;

    // Vulkan is available, at least for compute
    if (!glfwVulkanSupported())
        throw std::runtime_error("GLFW does not support Vulkan!");

    // for Vulkan, use GLFW_NO_API
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // create a glfw window
    GLFWwindow *window = glfwCreateWindow(128, 128, "dummy", nullptr, nullptr);

    // This surface is a dummy one for picking physical device
    // create vulkan surface from glfw
    ErrorCheck(glfwCreateWindowSurface(instance, window, nullptr, &surface), "create a dummy window surface");

    glfwDestroyWindow(window);
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
        bool adequate = CheckDeviceExtensionSupport(device, deviceExtensions);
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

void Context::InitLogicalDevice(const ContextDesc &desc) {
    // query device extensions (portability subset)
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    for (const auto &extension : availableExtensions)
        if (std::string(extension.extensionName) == "VK_KHR_portability_subset")
            deviceExtensions.push_back(extension.extensionName);

    // update queue family indices
    queueFamilyIndices = FindQueueFamilyIndices(physicalDevice, surface);

    // uniquify queue family indices
    std::set<uint32_t> uniqueQueueFamilies;
    if (queueFamilyIndices.compute.has_value()) uniqueQueueFamilies.insert(queueFamilyIndices.compute.value());
    if (queueFamilyIndices.graphics.has_value()) uniqueQueueFamilies.insert(queueFamilyIndices.graphics.value());
    if (queueFamilyIndices.present.has_value()) uniqueQueueFamilies.insert(queueFamilyIndices.present.value());

    // prepare queue infos
    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t index : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // prepare device features
    VkPhysicalDeviceFeatures deviceFeatures {};

    // prepare device info
    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = nullptr;

    // enable validation if needed
    if (desc.validation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // create logical device
    ErrorCheck(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "create logical device");

    // retrieve compute queue if necessary
    if (queueFamilyIndices.compute.has_value()) {
        vkGetDeviceQueue(device, queueFamilyIndices.compute.value(), 0, &computeQueue);
    }

    // retrieve graphics queue if necessary
    if (queueFamilyIndices.graphics.has_value()) {
        vkGetDeviceQueue(device, queueFamilyIndices.graphics.value(), 0, &graphicsQueue);
    }

    // retrieve present queue if necessary
    if (queueFamilyIndices.present.has_value()) {
        vkGetDeviceQueue(device, queueFamilyIndices.present.value(), 0, &presentQueue);
    }
}

void Context::InitMemoryAllocator(const ContextDesc &desc) {
    (void) desc;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;

    ErrorCheck(vmaCreateAllocator(&allocatorInfo, &allocator), "create vma allocator");
}

void Context::WaitIdle() const {
    ErrorCheck(vkDeviceWaitIdle(device), "device wait idle");
}

Window* Context::GetWindow() const {
    return window;
}

VkDevice Context::GetDevice() const {
    return device;
}

VmaAllocator Context::GetMemoryAllocator() const {
    return allocator;
}

QueueFamilyIndices Context::GetQueueFamilyIndices() const {
    return queueFamilyIndices;
}

void Context::Execute(std::function<void(CommandBuffer*)> callback, VkQueueFlagBits queue) {
    RenderFrame frame(this);
    auto commandBuffer = frame.RequestCommandBuffer(queue);
    commandBuffer->Begin();
    callback(commandBuffer);
    commandBuffer->End();
    commandBuffer->Submit();
    WaitIdle();
}

void Context::Execute(std::function<void(RenderFrame*, CommandBuffer*)> callback, VkQueueFlagBits queue) {
    RenderFrame frame(this);
    auto commandBuffer = frame.RequestCommandBuffer(queue);
    auto renderFrame = SlimPtr<RenderFrame>(this);
    commandBuffer->Begin();
    callback(renderFrame.get(), commandBuffer);
    commandBuffer->End();
    commandBuffer->Submit();
    WaitIdle();
}
