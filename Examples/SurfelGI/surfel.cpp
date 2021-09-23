#include "surfel.h"

SurfelManager::SurfelManager(Device* device, uint32_t numSurfels) : device(device), numSurfels(numSurfels) {

    VkBufferUsageFlags surfelBufferUsages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                          | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    // 1. prepare surfel buffers
    std::vector<Surfel> surfels(numSurfels);
    surfelBuffer = SlimPtr<Buffer>(device, numSurfels * sizeof(Surfel), surfelBufferUsages, VMA_MEMORY_USAGE_GPU_ONLY);

    // 2. prepare surfel state buffer
    SurfelState surfelState;
    surfelState.availableSurfels = numSurfels;
    surfelState.surfelBaseAddress = device->GetDeviceAddress(surfelBuffer);

    // 3. copy data to buffer
    device->Execute([&](CommandBuffer* commandBuffer) {
        commandBuffer->CopyDataToBuffer(surfels, surfelBuffer);
        commandBuffer->CopyDataToBuffer(surfelState, surfelStateBuffer);
    });
}
