#ifndef SLIM_UTILITY_RENDERGRAPH_H
#define SLIM_UTILITY_RENDERGRAPH_H

#include <string>
#include <vector>
#include <functional>

#include "core/vulkan.h"
#include "core/device.h"
#include "core/commands.h"
#include "core/image.h"
#include "core/renderpass.h"
#include "core/renderframe.h"
#include "utility/interface.h"

namespace slim {

    class RenderGraph;
    class RenderFrame;
    class CommandBuffer;

    struct RenderInfo {
        RenderGraph* renderGraph;
        RenderFrame* renderFrame;
        RenderPass* renderPass;
        CommandBuffer* commandBuffer;
    };

    class RenderGraph final : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        class Pass;
        class Resource;

        friend class Pass;
        friend class Resource;

        // -------------------------------------------------------------------------------

        class Resource final : public ReferenceCountable {
            friend class RenderGraph;
        public:
            explicit Resource(GPUImage *image);
            explicit Resource(VkFormat format, VkExtent2D extent, VkSampleCountFlagBits samples);
            virtual ~Resource() = default;
            Resource& SetMipLevels(uint32_t levels) { mipLevels = levels; return *this; }
            Image* GetImage() const { return image; }

            void UseAsColorBuffer()        { usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;         }
            void UseAsDepthBuffer()        { usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }
            void UseAsStencilBuffer()      { usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }
            void UseAsDepthStencilBuffer() { usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }
            void UseAsInputAttachment()    { usages |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;         }
            void UseAsStorage()            { usages |= VK_IMAGE_USAGE_STORAGE_BIT;                  }
            void UseAsTexture()            { usages |= VK_IMAGE_USAGE_SAMPLED_BIT;                  }

        private:
            void Allocate(RenderFrame* renderFrame);
            void Deallocate();

        private:
            VkFormat format;
            VkExtent2D extent;
            uint32_t mipLevels = 1;
            VkSampleCountFlagBits samples;
            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageUsageFlags usages = 0;

            Transient<GPUImage> image;
            std::vector<Pass*> readers = {};
            std::vector<Pass*> writers = {};

            bool retained = false;
            int rdCount = 0;
            int wrCount = 0;
        };

        // -------------------------------------------------------------------------------

        enum class ResourceType {
            ColorAttachment,
            ColorResolveAttachment,
            DepthAttachment,
            StencilAttachment,
            DepthStencilAttachment,
            PreserveAttachment,
            InputAttachment,
        };

        struct ResourceMetadata {
            Resource *resource;
            ResourceType type;
            std::optional<VkClearValue> clearValue = std::nullopt;
        };

        enum StorageImageUsage {
            STORAGE_IMAGE_READ_BIT = 1,
            STORAGE_IMAGE_WRITE_BIT = 2,
            STORAGE_IMAGE_READ_WRITE = STORAGE_IMAGE_READ_BIT | STORAGE_IMAGE_WRITE_BIT,
            STORAGE_IMAGE_READ_ONLY = STORAGE_IMAGE_READ_BIT,
            STORAGE_IMAGE_WRITE_ONLY = STORAGE_IMAGE_WRITE_BIT,
        };

        // -------------------------------------------------------------------------------

        class Subpass : public ReferenceCountable {
            friend class Pass;
        public:
            explicit Subpass(Pass* parent);

            // color resolve attachment
            void SetColorResolve(RenderGraph::Resource *resource);

            // preserved attachmented
            void SetPreserve(RenderGraph::Resource* resource);

            // input attachment
            void SetInput(RenderGraph::Resource* resource);

            // color attachment
            void SetColor(RenderGraph::Resource *resource);
            void SetColor(RenderGraph::Resource *resource, const ClearValue &clear);

            // separate depth
            void SetDepth(RenderGraph::Resource *resource);
            void SetDepth(RenderGraph::Resource *resource, const ClearValue &clear);

            // separate stencil
            void SetStencil(RenderGraph::Resource *resource);
            void SetStencil(RenderGraph::Resource *resource, const ClearValue &clear);

            // combined depth stencil
            void SetDepthStencil(RenderGraph::Resource *resource);
            void SetDepthStencil(RenderGraph::Resource *resource, const ClearValue &clear);

            // non-attachments
            void SetTexture(RenderGraph::Resource *resource); // texture image, read-only
            void SetStorage(RenderGraph::Resource *resource, uint32_t usage = STORAGE_IMAGE_READ_WRITE); // storage image

            void Execute(std::function<void(const RenderInfo &renderInfo)> callback);

        private:
            Pass* parent;
            std::function<void(const RenderInfo &renderTools)> callback;

            // attachments
            std::vector<uint32_t> usedAsColorAttachment = {};
            std::vector<uint32_t> usedAsColorResolveAttachment = {};
            std::vector<uint32_t> usedAsDepthAttachment = {};
            std::vector<uint32_t> usedAsStencilAttachment = {};
            std::vector<uint32_t> usedAsDepthStencilAttachment = {};
            std::vector<uint32_t> usedAsPreserveAttachment = {};
            std::vector<uint32_t> usedAsInputAttachment = {};
            std::vector<uint32_t> usedAsTexture = {};
            std::vector<uint32_t> usedAsStorage = {};
        };

        // -------------------------------------------------------------------------------

        class Pass : public ReferenceCountable {
            friend class Subpass;
            friend class RenderGraph;
        public:
            explicit Pass(const std::string &name, RenderGraph *graph, bool compute = false);

            Subpass* CreateSubpass();

            // color resolve attachment
            void SetColorResolve(RenderGraph::Resource* resource);

            // preserved attachment
            void SetPreserve(RenderGraph::Resource* resource);

            // input attachment
            void SetInput(RenderGraph::Resource* resource);

            // color attachment
            void SetColor(RenderGraph::Resource* resource);
            void SetColor(RenderGraph::Resource* resource, const ClearValue& clear);

            // separate depth
            void SetDepth(RenderGraph::Resource* resource);
            void SetDepth(RenderGraph::Resource* resource, const ClearValue& clear);

            // separate stencil
            void SetStencil(RenderGraph::Resource* resource);
            void SetStencil(RenderGraph::Resource* resource, const ClearValue& clear);

            // combined depth stencil
            void SetDepthStencil(RenderGraph::Resource* resource);
            void SetDepthStencil(RenderGraph::Resource* resource, const ClearValue& clear);

            // non-attachments
            void SetTexture(RenderGraph::Resource* resource);   // texture image
            void SetStorage(RenderGraph::Resource *resource, uint32_t usage = STORAGE_IMAGE_READ_WRITE);   // storage image

            void Execute(std::function<void(const RenderInfo& renderInfo)> callback);

        private:
            void Execute(CommandBuffer* commandBuffer);
            void ExecuteGraphics(CommandBuffer* commandBuffer);
            void ExecuteCompute(CommandBuffer* commandBuffer);

            uint32_t AddAttachment(const RenderGraph::ResourceMetadata& metadata);
            uint32_t AddTexture(Resource* resource);
            uint32_t AddStorage(Resource* resource);

            void TransitTextureLayout(Resource* resource);
            void TransitStorageLayout(Resource* resource);

        private:
            std::string name;
            RenderGraph *graph;

            bool compute = false;
            SmartPtr<Semaphore> signalSemaphore = nullptr;
            SmartPtr<CommandBuffer> commandBuffer = nullptr;

            // runtime decision
            bool visited = false;
            bool retained = false;
            bool useDefaultSubpass = true;

            std::vector<SmartPtr<Subpass>> subpasses = {};
            SmartPtr<Subpass> defaultSubpass = nullptr;

            // attachments
            std::vector<ResourceMetadata> attachments = {};
            std::unordered_map<Resource*, uint32_t> attachmentMap = {};

            // texture does not participate in render pass creation (not attachment)
            std::vector<Resource*> textures = {};
            std::unordered_map<Resource*, uint32_t> textureMap = {};

            // storage does not participate in render pass creation (not attachment)
            std::vector<Resource*> storages = {};
            std::unordered_map<Resource*, uint32_t> storageMap = {};
        };

        // -------------------------------------------------------------------------------

        explicit RenderGraph(RenderFrame *frame);
        virtual ~RenderGraph();

        RenderGraph::Pass*     CreateRenderPass(const std::string &name);
        RenderGraph::Pass*     CreateComputePass(const std::string &name);                                         // async compute
        RenderGraph::Resource* CreateResource(GPUImage* image);                                                    // for retained resource
        RenderGraph::Resource* CreateResource(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits samples);  // for transient resource
        void                   Compile();
        void                   Execute();
        void                   Visualize();
        void                   Print();

        RenderPass*            GetRenderPass() const;
        RenderFrame*           GetRenderFrame() const;
        CommandBuffer*         GetCommandBuffer() const;

    private:
        void                   CompilePass(Pass *pass);
        void                   CompileResource(Resource *resource);
        std::unordered_set<Pass*> FindPassDependencies(Pass* pass);

    private:
        SmartPtr<RenderFrame> renderFrame;
        mutable SmartPtr<RenderPass> renderPass;
        mutable SmartPtr<CommandBuffer> commandBuffer;
        std::vector<SmartPtr<RenderGraph::Pass>> passes = {};
        std::vector<SmartPtr<RenderGraph::Resource>> resources = {};
        std::vector<RenderGraph::Pass*> timeline = {};
        bool compiled = false;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_RENDERGRAPH_H
