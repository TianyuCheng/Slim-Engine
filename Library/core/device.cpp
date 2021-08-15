#include "core/device.h"
#include "core/debug.h"
#include "core/commands.h"
#include "core/renderframe.h"

using namespace slim;

Device::Device(Context *context) : context(context) {
    InitLogicalDevice();
    InitMemoryAllocator();
}

Device::~Device() {
    WaitIdle();

    // clean up memory allocator
    if (allocator) {
        vmaDestroyAllocator(allocator);
        allocator = VK_NULL_HANDLE;
    }

    // clean up logical device
    if (handle) {
        vkDestroyDevice(handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}

void Device::InitLogicalDevice() {
    const ContextDesc& desc = context->GetDescription();
    VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();
    VkSurfaceKHR surface = context->GetSurface();

    std::vector<const char*> deviceExtensions(desc.deviceExtensions.begin(), desc.deviceExtensions.end());

    // query device extensions (portability subset)
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    for (const auto &extension : availableExtensions) {
        if (std::string(extension.extensionName) == "VK_KHR_portability_subset") {
            deviceExtensions.push_back(extension.extensionName);
        }
    }

    // list all device extensions
    if (deviceExtensions.size()) {
        std::cout << "[CreateDevice] with extensions: " << std::endl;
        for (auto &extension : deviceExtensions) {
            std::cout << "- " << extension << std::endl;
        }
    }

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
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    const VkPhysicalDeviceFeatures2 &features = desc.features;

    // prepare device info
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = &features;

    // enable validation if needed
    if (desc.validation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(desc.validationLayers.size());
        createInfo.ppEnabledLayerNames = desc.validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // create logical device
    ErrorCheck(vkCreateDevice(physicalDevice, &createInfo, nullptr, &handle), "create logical device");

    // retrieve compute queue if necessary
    if (queueFamilyIndices.compute.has_value()) {
        vkGetDeviceQueue(handle, queueFamilyIndices.compute.value(), 0, &computeQueue);
    }

    // retrieve graphics queue if necessary
    if (queueFamilyIndices.graphics.has_value()) {
        vkGetDeviceQueue(handle, queueFamilyIndices.graphics.value(), 0, &graphicsQueue);
    }

    // retrieve present queue if necessary
    if (queueFamilyIndices.present.has_value()) {
        vkGetDeviceQueue(handle, queueFamilyIndices.present.value(), 0, &presentQueue);
    }

    // retrieve transfer queue if necessary
    if (queueFamilyIndices.transfer.has_value()) {
        vkGetDeviceQueue(handle, queueFamilyIndices.transfer.value(), 0, &transferQueue);
    }
}

void Device::InitMemoryAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = context->GetPhysicalDevice();
    allocatorInfo.device = handle;
    allocatorInfo.instance = context->GetInstance();

    ErrorCheck(vmaCreateAllocator(&allocatorInfo, &allocator), "create vma allocator");
}

void Device::WaitIdle() const {
    ErrorCheck(vkDeviceWaitIdle(handle), "device wait idle");
}

Context* Device::GetContext() const {
    return context;
}

VmaAllocator Device::GetMemoryAllocator() const {
    return allocator;
}

QueueFamilyIndices Device::GetQueueFamilyIndices() const {
    return queueFamilyIndices;
}

void Device::Execute(std::function<void(CommandBuffer*)> callback, VkQueueFlagBits queue) {
    RenderFrame frame(this);
    auto commandBuffer = frame.RequestCommandBuffer(queue);
    commandBuffer->Begin();
    callback(commandBuffer);
    commandBuffer->End();
    commandBuffer->Submit();
    WaitIdle();
}

void Device::Execute(std::function<void(RenderFrame*, CommandBuffer*)> callback, VkQueueFlagBits queue) {
    RenderFrame frame(this);
    auto commandBuffer = frame.RequestCommandBuffer(queue);
    auto renderFrame = SlimPtr<RenderFrame>(this);
    commandBuffer->Begin();
    callback(renderFrame.get(), commandBuffer);
    commandBuffer->End();
    commandBuffer->Submit();
    WaitIdle();
}
