#ifndef SLIM_UTILITY_RTBUILDER_H
#define SLIM_UTILITY_RTBUILDER_H

#include <list>
#include <vector>
#include "core/commands.h"
#include "core/acceleration.h"

namespace slim::scene {
    class Mesh;
    class Node;
}

namespace slim::accel {

    class Builder : public NotCopyable, public NotMovable, public ReferenceCountable {
    public:
        explicit Builder(Device* device);

        void EnableCompaction();

        uint32_t AddMesh(scene::Mesh* mesh);
        uint32_t AddAABBs(Buffer* aabbsBuffer, uint32_t count, uint32_t stride);
        void     AddNode(scene::Node* node, uint32_t sbtRecordOffset = 0, uint32_t mask = 0xff);

        void BuildTlas();
        void BuildBlas();

        AccelStruct* GetTlas()               const { return tlas->accel;        }
        AccelStruct* GetBlas(uint32_t index) const { return blas[index]->accel; }

        accel::Instance* GetTlasInstance()               const { return tlas;        }
        accel::Geometry* GetBlasGeometry(uint32_t index) const { return blas[index]; }

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

        // configuration
        bool compaction = false;

        // acceleration structure
        SmartPtr<Instance> tlas;
        std::vector<SmartPtr<Geometry>> blas;
    };

} // end of slim namespace

#endif // SLIM_UTILITY_RTBUILDER_H
