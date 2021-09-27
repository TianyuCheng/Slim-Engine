#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#endif

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/context.h"
#include "core/debug.h"

using namespace slim;

template <typename T>
void AddToFeatures(VkPhysicalDeviceFeatures2* features, T* object) {
    object->pNext = features->pNext;
    features->pNext = object;
}

template <typename T>
void AddToProperties(VkPhysicalDeviceProperties2* properties, T* object) {
    object->pNext = properties->pNext;
    properties->pNext = object;
}

ContextDesc::ContextDesc() {
    // initialize physical device features
    features.reset(new VkPhysicalDeviceFeatures2{ });
    features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features->pNext = nullptr;
    // debug extensions
    debugExtensions.insert(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
}

ContextDesc& ContextDesc::Verbose(bool value) {
    verbose = value;
    return *this;
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

ContextDesc& ContextDesc::EnableNonSolidPolygonMode() {
    features->features.fillModeNonSolid = VK_TRUE;
    return *this;
}

ContextDesc& ContextDesc::EnableSeparateDepthStencilLayout() {
    // enable separate depth stencil layout
    if (!deviceFeatures.separateDepthStencilLayout.get()) {
        deviceFeatures.separateDepthStencilLayout.reset(new VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures { });
        deviceFeatures.separateDepthStencilLayout->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
        deviceFeatures.separateDepthStencilLayout->separateDepthStencilLayouts = VK_TRUE;
        deviceFeatures.separateDepthStencilLayout->pNext = nullptr;
        AddToFeatures(features.get(), deviceFeatures.separateDepthStencilLayout.get());
    }
    return *this;
}

ContextDesc& ContextDesc::EnableDescriptorIndexing() {
    // add instance extensions
    instanceExtensions.insert(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // add device extensions
    deviceExtensions.insert(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    deviceExtensions.insert(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

    // add physical device features
    if (!deviceFeatures.descriptorIndexing.get()) {
        deviceFeatures.descriptorIndexing.reset(new VkPhysicalDeviceDescriptorIndexingFeatures { });
        deviceFeatures.descriptorIndexing->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        deviceFeatures.descriptorIndexing->shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        deviceFeatures.descriptorIndexing->runtimeDescriptorArray = VK_TRUE;
        deviceFeatures.descriptorIndexing->descriptorBindingVariableDescriptorCount = VK_TRUE;
        deviceFeatures.descriptorIndexing->descriptorBindingPartiallyBound = VK_TRUE;
        deviceFeatures.descriptorIndexing->pNext = nullptr;
        AddToFeatures(features.get(), deviceFeatures.descriptorIndexing.get());
    }
    return *this;
}

ContextDesc& ContextDesc::EnableRayTracing() {
    // adding required device extensions
    deviceExtensions.insert(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    deviceExtensions.insert(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    // deviceExtensions.insert(VK_KHR_RAY_QUERY_EXTENSION_NAME); // This requires RTX GPU
    deviceExtensions.insert(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    deviceExtensions.insert(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    deviceExtensions.insert(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);

    // adding acceleration structure feature
    if (!deviceFeatures.accelerationStructure.get()) {
        deviceFeatures.accelerationStructure.reset(new VkPhysicalDeviceAccelerationStructureFeaturesKHR { });
        deviceFeatures.accelerationStructure->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        deviceFeatures.accelerationStructure->accelerationStructure = VK_TRUE;
        deviceFeatures.accelerationStructure->pNext = nullptr;
        AddToFeatures(features.get(), deviceFeatures.accelerationStructure.get());
    }

    // adding ray tracing pipeline features
    if (!deviceFeatures.rayTracingPipeline.get()) {
        deviceFeatures.rayTracingPipeline.reset(new VkPhysicalDeviceRayTracingPipelineFeaturesKHR { });
        deviceFeatures.rayTracingPipeline->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        deviceFeatures.rayTracingPipeline->rayTracingPipeline = VK_TRUE;
        deviceFeatures.rayTracingPipeline->rayTraversalPrimitiveCulling = VK_TRUE;
        deviceFeatures.rayTracingPipeline->pNext = nullptr;
        AddToFeatures(features.get(), deviceFeatures.rayTracingPipeline.get());
    }

    #if 0 // This requires RTX GPU
    // adding ray query pipeline features
    if (!deviceFeatures.rayQuery.get()) {
        deviceFeatures.rayQuery.reset(new VkPhysicalDeviceRayQueryFeaturesKHR {});
        deviceFeatures.rayQuery->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        deviceFeatures.rayQuery->rayQuery = VK_TRUE;
        deviceFeatures.rayQuery->pNext = nullptr;
        AddToFeatures(features.get(), deviceFeatures.rayQuery.get());
    }
    #endif

    // adding host query reset features
    if (!deviceFeatures.hostQueryReset.get()) {
        deviceFeatures.hostQueryReset.reset(new VkPhysicalDeviceHostQueryResetFeatures { });
        deviceFeatures.hostQueryReset->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
        deviceFeatures.hostQueryReset->hostQueryReset = VK_TRUE;
        deviceFeatures.hostQueryReset->pNext = nullptr;
        AddToFeatures(features.get(), deviceFeatures.hostQueryReset.get());
    }

    return *this;
}

ContextDesc& ContextDesc::EnableBufferDeviceAddress() {
    // add device extension
    deviceExtensions.insert(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    // adding buffer device address features
    if (!deviceFeatures.bufferDeviceAddress.get()) {
        deviceFeatures.bufferDeviceAddress.reset(new VkPhysicalDeviceBufferDeviceAddressFeatures { });
        deviceFeatures.bufferDeviceAddress->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        deviceFeatures.bufferDeviceAddress->bufferDeviceAddress = VK_TRUE;
        AddToFeatures(features.get(), deviceFeatures.bufferDeviceAddress.get());
    }

    return *this;
}

ContextDesc& ContextDesc::EnableShaderInt64() {
    features->features.shaderInt64 = VK_TRUE;
    return *this;
}

ContextDesc& ContextDesc::EnableShaderFloat64() {
    features->features.shaderFloat64 = VK_TRUE;
    return *this;
}

ContextDesc& ContextDesc::EnableMultiDraw() {
    features->features.multiDrawIndirect = VK_TRUE;
    return *this;
}

void ContextDesc::PrepareForGlfw() {
    glfwInit();
    // query for glfw extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        instanceExtensions.insert(glfwExtensions[i]);
    }
    glfwTerminate();
}

void ContextDesc::PrepareForViewport() {
    // MAINTENANCE1: contains the extension to allow negative height on viewport.
    // This would allow an OpenGL style viewport.
    deviceExtensions.insert(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
}

void ContextDesc::PrepareForSwapchain() {
    // SWAPCHAIN: contains the extension for swapchain for presentation
    deviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void ContextDesc::PrepareForValidation() {
    validationLayers.insert("VK_LAYER_KHRONOS_validation");
    instanceExtensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // TODO: need to query validation layer support
    const auto &supportedInstanceExtensions = GetSupportedInstanceExtensions();

    // list all instance layers
    if (validationLayers.size()) {
        if (verbose) {
            std::cout << "[CreateInstance] with validation layers: " << std::endl;
            for (auto &layer : validationLayers)
                std::cout << "- " << layer << std::endl;
        }
    }

    // list all instance extensions
    if (instanceExtensions.size()) {
        if (verbose) {
            std::cout << "[CreateInstance] with instance extensions: " << std::endl;
        }
        std::vector<std::string> extensionsNotFound = {};
        for (auto &extension : instanceExtensions) {
            bool found = supportedInstanceExtensions.find(std::string(extension)) != supportedInstanceExtensions.end();
            if (verbose) {
                std::cout << "- " << extension << ": ";
                std::cout << (found ? "FOUND" : "NOT FOUND") << std::endl;
            }
            if (!found) {
                extensionsNotFound.push_back(std::string(extension));
            }
        }
        for (auto& extension : extensionsNotFound) {
            std::cerr << "Error: " << extension << ": NOT FOUND" << std::endl;
        }
        if (!extensionsNotFound.empty()) {
            std::cerr << "[ERROR] FAILED TO FIND ALL INSTANCE EXTENSIONS" << std::endl;
            throw std::runtime_error("Failed to find all instance extensions!");
        }
    }
}

Context::Context(const ContextDesc& desc) : desc(desc) {
    // initialize glfw
    if (desc.present) {
        glfwInit();
    }

    InitInstance(desc);
    InitDebuggerMessener(desc);
    InitSurface(desc);
    InitPhysicalDevice(desc);

    // debug
    #if 0
    struct FeatureStruct {
        VkStructureType sType;
        void* pNext;
    };
    void* pNext = nullptr;
    pNext = this->desc.features->pNext;
    while (pNext) {
        auto* feature = (FeatureStruct*)pNext;
        std::cout << "sType: " << feature->sType << std::endl;
        pNext = feature->pNext;
    }
    #endif
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

    // clean up glfw
    if (desc.present) {
        glfwTerminate();
    }
}

void Context::InitInstance(const ContextDesc& desc) {
    // prepare extensions and layers
    instanceExtensions.clear();
    for (const std::string& name : desc.instanceExtensions) {
        instanceExtensions.push_back(name.c_str());
    }

    deviceExtensions.clear();
    for (const std::string& name : desc.deviceExtensions) {
        deviceExtensions.push_back(name.c_str());
    }

    validationLayers.clear();
    for (const std::string& name : desc.validationLayers) {
        validationLayers.push_back(name.c_str());
    }

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

    // load instsance functions
    volkLoadInstance(instance);
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
    surface = VK_NULL_HANDLE;

    if (!desc.present) {
        return;
    }

    // Vulkan is available, at least for compute
    if (!glfwVulkanSupported()) {
        std::cerr << "[ERROR] GLFW does not support vulkan" << std::endl;
        throw std::runtime_error("GLFW does not support Vulkan!");
    }

    // for Vulkan, use GLFW_NO_API
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // create a glfw window
    GLFWwindow *window = glfwCreateWindow(128, 128, "dummy", nullptr, nullptr);

    // This surface is a dummy one for picking physical device
    // create vulkan surface from glfw
    ErrorCheck(glfwCreateWindowSurface(instance, window, nullptr, &surface), "create a dummy window surface");

    // clean up window
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
        std::cerr << "[ERROR] FAILED TO FIND GPUS WITH VULKAN SUPPORT" << std::endl;
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    // get all physical device objects
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // store scores of all devices
    std::vector<std::pair<VkPhysicalDevice, int>> candidates;
    for (const auto& device : devices) {
        auto unsupported = CheckDeviceExtensionSupport(device, deviceExtensions);
        if (unsupported.empty()) {
            int score = rateDevice(device, surface);
            candidates.push_back(std::make_pair(device, score));
        }
    }

    // check available device candidates
    if (candidates.empty()) {
        std::cerr << "[ERROR] DEVICE EXTENSIONS NOT SUPPORTED" << std::endl;
        for (const auto& device : devices) {
            std::cerr << "GPU: " << std::endl;
            auto unsupported = CheckDeviceExtensionSupport(device, deviceExtensions);
            for (auto& extension : unsupported) {
                std::cerr << "[UNSUPPORTED] " << extension << std::endl;
            }
        }
        throw std::runtime_error("Device extensions not supported!");
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
        std::cerr << "[ERROR] FAILED TO FIND A SUITABLE GPU" << std::endl;
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR Context::GetRayTracingPipelineProperties() {
    if (rayTracingPipelineProperties.sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR) {
        return rayTracingPipelineProperties;
    }
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 prop2 = {};
    prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    prop2.pNext = &rayTracingPipelineProperties;

    vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);
    return rayTracingPipelineProperties;
}
