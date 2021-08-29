#ifndef SLIM_UTILITY_RTBUILDER_H
#define SLIM_UTILITY_RTBUILDER_H

#include <list>
#include <vector>
#include "core/commands.h"
#include "core/acceleration.h"
#include "utility/scenegraph.h"

namespace slim::accel {

    class Builder : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        explicit Builder(Device* device);

        void EnableCompaction();

        void AddMesh(Mesh* mesh, uint64_t vertexStride);
        void AddNode(scene::Node* node);

        void BuildTlas();
        void BuildBlas();

    private:

        void CreateTlas(CommandBuffer* commandBuffer,
                        VkDeviceAddress scratchAddress);
        void CreateBlas(CommandBuffer* commandBuffer,
                        const std::vector<uint32_t> &indices,
                        VkDeviceAddress scratchAddress, QueryPool* queryPool);
        void CompactBlas(CommandBuffer* commandBuffer,
                         const std::vector<uint32_t> &indices,
                         QueryPool* queryPool);
        void CleanNoncompacted(const std::vector<uint32_t> &indices);

    private:
        SmartPtr<Device> device;
        bool compaction = false;
        std::vector<SmartPtr<Instance>> tlas;
        std::vector<SmartPtr<Geometry>> blas;
    };

} // end of slim namespace

#endif // SLIM_UTILITY_RTBUILDER_H
