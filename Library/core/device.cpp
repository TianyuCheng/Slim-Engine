#include "core/device.h"
#include "core/debug.h"
#include "core/commands.h"
#include "core/buffer.h"
#include "core/acceleration.h"
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

    std::vector<const char*> deviceExtensions;
    for (const std::string& name : desc.deviceExtensions) {
        deviceExtensions.push_back(name.c_str());
    }

    std::vector<const char*> validationLayers;
    for (const std::string& name : desc.validationLayers) {
        validationLayers.push_back(name.c_str());
    }

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
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // create logical device
    ErrorCheck(vkCreateDevice(physicalDevice, &createInfo, nullptr, &handle), "create logical device");
    volkLoadDeviceTable(&deviceTable, handle);

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
    // need to manually map functions to vma
    VmaVulkanFunctions vulkanFunctions;
    vulkanFunctions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    vulkanFunctions.vkAllocateMemory                        = deviceTable.vkAllocateMemory;
    vulkanFunctions.vkFreeMemory                            = deviceTable.vkFreeMemory;
    vulkanFunctions.vkMapMemory                             = deviceTable.vkMapMemory;
    vulkanFunctions.vkUnmapMemory                           = deviceTable.vkUnmapMemory;
    vulkanFunctions.vkFlushMappedMemoryRanges               = deviceTable.vkFlushMappedMemoryRanges;
    vulkanFunctions.vkInvalidateMappedMemoryRanges          = deviceTable.vkInvalidateMappedMemoryRanges;
    vulkanFunctions.vkBindBufferMemory                      = deviceTable.vkBindBufferMemory;
    vulkanFunctions.vkBindImageMemory                       = deviceTable.vkBindImageMemory;
    vulkanFunctions.vkGetBufferMemoryRequirements           = deviceTable.vkGetBufferMemoryRequirements;
    vulkanFunctions.vkGetImageMemoryRequirements            = deviceTable.vkGetImageMemoryRequirements;
    vulkanFunctions.vkCreateBuffer                          = deviceTable.vkCreateBuffer;
    vulkanFunctions.vkDestroyBuffer                         = deviceTable.vkDestroyBuffer;
    vulkanFunctions.vkCreateImage                           = deviceTable.vkCreateImage;
    vulkanFunctions.vkDestroyImage                          = deviceTable.vkDestroyImage;
    vulkanFunctions.vkCmdCopyBuffer                         = deviceTable.vkCmdCopyBuffer;
    vulkanFunctions.vkGetBufferMemoryRequirements2KHR       = deviceTable.vkGetBufferMemoryRequirements2KHR;
    vulkanFunctions.vkGetImageMemoryRequirements2KHR        = deviceTable.vkGetImageMemoryRequirements2KHR;
    vulkanFunctions.vkBindBufferMemory2KHR                  = deviceTable.vkBindBufferMemory2KHR;
    vulkanFunctions.vkBindImageMemory2KHR                   = deviceTable.vkBindImageMemory2KHR;

    // create allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = context->GetPhysicalDevice();
    allocatorInfo.device = handle;
    allocatorInfo.instance = context->GetInstance();
    allocatorInfo.flags = 0;
    allocatorInfo.pVulkanFunctions = (const VmaVulkanFunctions*) &vulkanFunctions;

    // enable buffer device address capability for vma allocator
    if (context->GetDescription().deviceFeatures.bufferDeviceAddress) {
        allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }

    ErrorCheck(vmaCreateAllocator(&allocatorInfo, &allocator), "create vma allocator");
}

void Device::WaitIdle() const {
    ErrorCheck(deviceTable.vkDeviceWaitIdle(handle), "device wait idle");
}

void Device::WaitIdle(VkQueueFlagBits queue) const {
    switch (queue) {
        case VK_QUEUE_COMPUTE_BIT:
            ErrorCheck(deviceTable.vkQueueWaitIdle(computeQueue), "compute queue wait idle");
            break;
        case VK_QUEUE_GRAPHICS_BIT:
            ErrorCheck(deviceTable.vkQueueWaitIdle(graphicsQueue), "graphics queue wait idle");
            break;
        case VK_QUEUE_TRANSFER_BIT:
            ErrorCheck(deviceTable.vkQueueWaitIdle(transferQueue), "transfer queue wait idle");
            break;
        default:
            throw std::runtime_error("invalid queue for wait idle");
    }
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

VkDeviceAddress Device::GetDeviceAddress(Buffer* buffer) const {
    VkBufferDeviceAddressInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = *buffer;
    return deviceTable.vkGetBufferDeviceAddress(handle, &info);
}

VkDeviceAddress Device::GetDeviceAddress(accel::AccelStruct* as) const {
    VkAccelerationStructureDeviceAddressInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    info.accelerationStructure = *as;
    return deviceTable.vkGetAccelerationStructureDeviceAddressKHR(handle, &info);
}

void Device::Execute(std::function<void(CommandBuffer*)> callback, VkQueueFlagBits queue) {
    RenderFrame frame(this);
    auto commandBuffer = frame.RequestCommandBuffer(queue);
    commandBuffer->Begin();
    callback(commandBuffer);
    commandBuffer->End();
    commandBuffer->Submit();
    WaitIdle(queue);
}

void Device::Execute(std::function<void(RenderFrame*, CommandBuffer*)> callback, VkQueueFlagBits queue) {
    RenderFrame frame(this);
    auto commandBuffer = frame.RequestCommandBuffer(queue);
    auto renderFrame = SlimPtr<RenderFrame>(this);
    commandBuffer->Begin();
    callback(renderFrame.get(), commandBuffer);
    commandBuffer->End();
    commandBuffer->Submit();
    WaitIdle(queue);
}
