#ifndef SLIM_CORE_CONTEXT_H
#define SLIM_CORE_CONTEXT_H

#include <string>
#include <vector>
#include <functional>
#include <unordered_set>

#include "core/vulkan.h"
#include "core/vkutils.h"
#include "utility/interface.h"

namespace slim {

    class Device;
    class Context;

    // ContextDesc should hold the data for all configurations needed for context.
    // When context is initialized, nothing should be changeable.
    class ContextDesc final {
        friend class Context;
        friend class Device;
    public:
        explicit ContextDesc();
        virtual ~ContextDesc() = default;

        ContextDesc& Verbose(bool value = true);

        // configure basic requirements
        ContextDesc& EnableValidation(bool value = true);
        ContextDesc& EnableGraphics(bool value = true);
        ContextDesc& EnableCompute(bool value = true);
        ContextDesc& EnableGLFW(bool value = true);

        // configure device features
        ContextDesc& EnableSeparateDepthStencilLayout();
        ContextDesc& EnableDescriptorIndexing();
        ContextDesc& EnableNonSolidPolygonMode();
        ContextDesc& EnableShaderInt64();
        ContextDesc& EnableShaderFloat64();
        ContextDesc& EnableRayTracing();
        ContextDesc& EnableBufferDeviceAddress();

        // allow finer-grain tuning by users
        VkPhysicalDeviceFeatures& GetDeviceFeatures() {
            return features->features;
        }
        VkPhysicalDeviceDescriptorIndexingFeatures& GetDescriptorIndexingFeatures() {
            return *deviceFeatures.descriptorIndexing;
        }
        VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures& GetSeparateDepthStencilLayoutFeatures() {
            return *deviceFeatures.separateDepthStencilLayout;
        }

    private:
        void PrepareForGlfw();
        void PrepareForViewport();
        void PrepareForSwapchain();
        void PrepareForValidation();

    private:
        bool validation = false;
        bool graphics = false;
        bool compute = false;
        bool present = false;
        bool verbose = false;

        // physical device features
        std::shared_ptr<VkPhysicalDeviceFeatures2> features = {};
        struct {
            std::shared_ptr<VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures> separateDepthStencilLayout;
            std::shared_ptr<VkPhysicalDeviceDescriptorIndexingFeatures> descriptorIndexing;
            std::shared_ptr<VkPhysicalDeviceBufferDeviceAddressFeatures> bufferDeviceAddress;
            std::shared_ptr<VkPhysicalDeviceAccelerationStructureFeaturesKHR> accelerationStructure;
            std::shared_ptr<VkPhysicalDeviceRayTracingPipelineFeaturesKHR> rayTracingPipeline;
            std::shared_ptr<VkPhysicalDeviceRayQueryFeaturesKHR> rayQuery;
            std::shared_ptr<VkPhysicalDeviceHostQueryResetFeatures> hostQueryReset;
        } deviceFeatures;

        // extensions + validation layers
        // clean up glfw
        std::unordered_set<std::string> instanceExtensions = {};
        std::unordered_set<std::string> validationLayers = {};
        std::unordered_set<std::string> deviceExtensions = {};
        std::unordered_set<std::string> debugExtensions = {};
    };

    class Context final : public NotCopyable, public NotMovable, public ReferenceCountable {
        friend class Context;
    public:
        explicit Context(const ContextDesc& desc);
        virtual ~Context();

        VkInstance GetInstance() const { return instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        VkSurfaceKHR GetSurface() const { return surface; }
        const ContextDesc& GetDescription() const { return desc; }

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR
            GetRayTracingPipelineProperties();

    private:
        void InitInstance(const ContextDesc &desc);
        void InitDebuggerMessener(const ContextDesc &desc);
        void InitSurface(const ContextDesc &desc);
        void InitPhysicalDevice(const ContextDesc &desc);

    private:
        ContextDesc              desc;
        VkInstance               instance       = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR             surface        = VK_NULL_HANDLE;
        VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;

        std::vector<const char*> instanceExtensions;
        std::vector<const char*> deviceExtensions;
        std::vector<const char*> validationLayers;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties = {};
    };

} // end of namespace slim

#endif // end of SLIM_CORE_CONTEXT_H
