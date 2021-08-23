#include <slim/slim.hpp>

using namespace slim;

struct Material {
    alignas(16) glm::vec3 baseColor;
    alignas(16) glm::vec3 emitColor;
    alignas(4)  float metalness;
    alignas(4)  float roughness;
};

struct GeometryData {
    uint32_t vertexCount;
    uint32_t indexCount;
    VkDeviceOrHostAddressConstKHR vertexData;
    VkDeviceOrHostAddressConstKHR indexData;
    VkDeviceOrHostAddressConstKHR transformData;
};

VkDeviceAddress GetDeviceAddress(VkDevice device, VkBuffer buffer) {
    VkBufferDeviceAddressInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = buffer;
    VkDeviceAddress address = vkGetBufferDeviceAddress(device, &info);
    return address;
}

int main() {
    // create a slim device
    auto context = SlimPtr<Context>(
        ContextDesc()
            .EnableCompute(true)
            .EnableGraphics(true)
            .EnableValidation(true)
            .EnableGLFW(true)
            .EnableRayTracing()
    );

    auto vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR) vkGetInstanceProcAddr(context->GetInstance(), "vkCmdBuildAccelerationStructuresKHR");
    auto vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR) vkGetInstanceProcAddr(context->GetInstance(), "vkCreateAccelerationStructureKHR");
    auto vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR) vkGetInstanceProcAddr(context->GetInstance(), "vkDestroyAccelerationStructureKHR");

    // create a slim device
    auto device = SlimPtr<Device>(context);

    // create a slim window
    auto window = SlimPtr<Window>(
        device,
        WindowDesc()
            .SetResolution(512, 512)
            .SetResizable(true)
            .SetTitle("Ray Tracing")
    );

    // create a scene camera
    auto camera = SlimPtr<Camera>("camera");
    camera->Perspective(1.05, 1.0, 0.1, 20.0);
    camera->LookAt(glm::vec3(0.0, 0.0, 3.0),
                   glm::vec3(0.0, 0.0, 0.0),
                   glm::vec3(0.0, 1.0, 0.0));

    VkBufferUsageFlags bufferFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaMemoryUsage memoryFlags = VMA_MEMORY_USAGE_GPU_ONLY;

    std::vector<::GeometryData> data;
    auto vertexBuffer    = SlimPtr<Buffer>(device, 65536ULL, bufferFlags | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, memoryFlags);
    auto indexBuffer     = SlimPtr<Buffer>(device, 65536ULL, bufferFlags | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, memoryFlags);
    auto materialBuffer  = SlimPtr<Buffer>(device, 65536ULL, bufferFlags, memoryFlags);
    auto transformBuffer = SlimPtr<Buffer>(device, 65536ULL, bufferFlags | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, memoryFlags);
    auto scratchBuffer   = SlimPtr<Buffer>(device, 65536ULL, bufferFlags, memoryFlags);
    auto blasBuffer      = SlimPtr<Buffer>(device, 65536ULL, bufferFlags | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, memoryFlags);
    device->Execute([&](CommandBuffer* commandBuffer) {
        auto sphereGeometry = Sphere { }.Create();

        auto sphereMaterial = ::Material { };
        sphereMaterial.baseColor = glm::vec3(1.0, 1.0, 1.0);
        sphereMaterial.emitColor = glm::vec3(0.0, 0.0, 0.0);
        sphereMaterial.metalness = 0.7;
        sphereMaterial.roughness = 0.3;

        // vertex buffer
        commandBuffer->CopyDataToBuffer(sphereGeometry.vertices, vertexBuffer);

        // index buffer
        commandBuffer->CopyDataToBuffer(sphereGeometry.indices, indexBuffer);

        // transform buffer
        commandBuffer->CopyDataToBuffer(glm::mat4(1.0), indexBuffer);

        // material buffer
        std::vector<::Material> materials = { sphereMaterial };
        commandBuffer->CopyDataToBuffer(materials, materialBuffer);

        // geometry data
        data.push_back(::GeometryData { });
        auto& datum = data.back();
        datum.vertexCount = sphereGeometry.vertices.size();
        datum.indexCount = sphereGeometry.indices.size();
        datum.vertexData = VkDeviceOrHostAddressConstKHR { };
        datum.vertexData.deviceAddress = GetDeviceAddress(*device, *vertexBuffer);
        datum.indexData = VkDeviceOrHostAddressConstKHR { };
        datum.indexData.deviceAddress = GetDeviceAddress(*device, *indexBuffer);
        datum.transformData = VkDeviceOrHostAddressConstKHR { };
        datum.transformData.deviceAddress = GetDeviceAddress(*device, *transformBuffer);
    });

    #if 1 // The dark side
    VkAccelerationStructureKHR blas = VK_NULL_HANDLE;
    VkAccelerationStructureCreateInfoKHR blasCreateInfo = {};
    blasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blasCreateInfo.buffer = *blasBuffer;
    blasCreateInfo.offset = 0;
    blasCreateInfo.deviceAddress = { };
    VkResult result = vkCreateAccelerationStructureKHR(*device, &blasCreateInfo, nullptr, &blas);
    assert(result == VK_SUCCESS);

    std::vector<VkAccelerationStructureGeometryKHR> geometries = {};
    for (const auto& datum : data) {
        VkAccelerationStructureGeometryDataKHR geometryData = {};
        geometryData.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometryData.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometryData.triangles.vertexStride = sizeof(slim::GeometryData);
        geometryData.triangles.vertexData = datum.vertexData;
        geometryData.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometryData.triangles.indexData = datum.indexData;
        geometryData.triangles.transformData = datum.transformData;

        geometries.push_back(VkAccelerationStructureGeometryKHR { });
        auto& geometry = geometries.back();
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry = geometryData;
        geometry.flags = 0;
    }

    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo = {};
    VkAccelerationStructureBuildRangeInfoKHR geometryBuildRangeInfo = {};
    {
        geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        geometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        geometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        geometryInfo.geometryCount = geometries.size();
        geometryInfo.pGeometries = geometries.data();
        geometryInfo.ppGeometries = nullptr;
        geometryInfo.scratchData = VkDeviceOrHostAddressKHR {};
        geometryInfo.scratchData.deviceAddress = GetDeviceAddress(*device, *scratchBuffer);
        geometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        geometryInfo.dstAccelerationStructure = blas;

        geometryBuildRangeInfo.primitiveCount = 1;
        geometryBuildRangeInfo.primitiveOffset = 0;
        geometryBuildRangeInfo.firstVertex = 0;
        geometryBuildRangeInfo.transformOffset = 0;
    }

    device->Execute([&](CommandBuffer* commandBuffer) {
        VkAccelerationStructureBuildRangeInfoKHR* buildRange = &geometryBuildRangeInfo;
        vkCmdBuildAccelerationStructuresKHR(*commandBuffer, 1, &geometryInfo, &buildRange);
    }, VK_QUEUE_GRAPHICS_BIT);
    #endif

    // while (window->IsRunning()) {
    //     Window::PollEvents();
    //
    //     // get current render frame
    //     auto frame = window->AcquireNext();
    //
    //     // update camera perspective matrix based on frame aspect ratio
    //     camera->Perspective(1.05, frame->GetAspectRatio(), 0.1, 20.0);
    // }

    vkDestroyAccelerationStructureKHR(*device, blas, nullptr);

    device->WaitIdle();
    return EXIT_SUCCESS;
}
