#ifndef SLIM_CORE_RENDERGRAPH_H
#define SLIM_CORE_RENDERGRAPH_H

#include <string>
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

#include "core/context.h"
#include "core/commands.h"
#include "core/image.h"
#include "core/renderpass.h"
#include "core/renderframe.h"
#include "utility/interface.h"

namespace slim {

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
            explicit Resource(GPUImage2D *image);
            explicit Resource(VkFormat format, VkExtent2D extent, VkSampleCountFlagBits samples);
            virtual ~Resource() = default;
            Resource& SetMipLevels(uint32_t levels) { mipLevels = levels; return *this; }
            Image* GetImage() const { return image; }
        private:
            void Allocate(RenderFrame* renderFrame);
            void Deallocate();

        private:
            VkFormat format;
            VkExtent2D extent;
            uint32_t mipLevels = 1;
            VkSampleCountFlagBits samples;
            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

            Transient<GPUImage2D> image;
            std::vector<Pass*> readers = {};
            std::vector<Pass*> writers = {};

            bool retained = false;
            int rdCount = 0;
            int wrCount = 0;
        };

        // -------------------------------------------------------------------------------

        class Pass : public ReferenceCountable {
            friend class RenderGraph;
        public:
            explicit Pass(const std::string &name, RenderGraph *graph, bool compute = false);

            void SetColor(RenderGraph::Resource *resource);
            void SetColor(RenderGraph::Resource *resource, const ClearValue &clear);

            void SetDepth(RenderGraph::Resource *resource);
            void SetDepth(RenderGraph::Resource *resource, const ClearValue &clear);

            void SetStencil(RenderGraph::Resource *resource);
            void SetStencil(RenderGraph::Resource *resource, const ClearValue &clear);

            void SetDepthStencil(RenderGraph::Resource *resource);
            void SetDepthStencil(RenderGraph::Resource *resource, const ClearValue &clear);

            void SetTexture(RenderGraph::Resource *resource);

            void Execute(std::function<void(const RenderGraph &renderGraph)> callback);

        private:
            void Execute();
            void ExecuteGraphics();
            void ExecuteCompute();

        private:
            struct ResourceMetadata {
                Resource *resource;
                std::optional<VkClearValue> clearValue = std::nullopt;
            };

            std::string name;
            RenderGraph *graph;

            bool compute = false;

            // runtime decision
            bool visited = false;
            bool retained = false;

            std::vector<Resource*> attachments = {};
            std::vector<ResourceMetadata> usedAsTexture = {};
            std::vector<ResourceMetadata> usedAsColorAttachment = {};
            std::vector<ResourceMetadata> usedAsDepthAttachment = {};
            std::vector<ResourceMetadata> usedAsStencilAttachment = {};
            std::vector<ResourceMetadata> usedAsDepthStencilAttachment = {};

            std::function<void(const RenderGraph &renderGraph)> callback;
        };

        // -------------------------------------------------------------------------------

        explicit RenderGraph(RenderFrame *frame);
        virtual ~RenderGraph();

        RenderGraph::Pass*     CreateRenderPass(const std::string &name);
        RenderGraph::Pass*     CreateComputePass(const std::string &name);                                         // async compute
        RenderGraph::Resource* CreateResource(GPUImage2D* image);                                                  // for retained resource
        RenderGraph::Resource* CreateResource(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits samples);  // for transient resource
        void                   Compile();
        void                   Execute();
        void                   Visualize();

        RenderPass*            GetRenderPass() const;
        RenderFrame*           GetRenderFrame() const;
        CommandBuffer*         GetComputeCommandBuffer() const;
        CommandBuffer*         GetGraphicsCommandBuffer() const;

    private:
        void                   CompilePass(Pass *pass);
        void                   CompileResource(Resource *resource);

    private:
        SmartPtr<RenderFrame> renderFrame;
        mutable SmartPtr<CommandBuffer> computeCommandBuffer;
        mutable SmartPtr<CommandBuffer> graphicsCommandBuffer;
        mutable SmartPtr<RenderPass> renderPass;
        std::vector<SmartPtr<RenderGraph::Pass>> passes = {};
        std::vector<SmartPtr<RenderGraph::Resource>> resources = {};
        std::vector<RenderGraph::Pass*> timeline = {};
        bool compiled = false;
    };

} // end of namespace slim

#endif // end of SLIM_CORE_RENDERGRAPH_H
