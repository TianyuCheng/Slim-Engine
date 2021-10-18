#include "update.h"

void AddUpdatePass(RenderGraph&       graph,
                   AutoReleasePool&   pool,
                   render::GBuffer*   gbuffer,
                   render::SceneData* sceneData,
                   render::Surfel*    surfel,
                   render::Debug*     debug,
                   Scene*             scene) {

    static uint32_t frameID = 0;

    auto pass = graph.CreateComputePass("scene-update");
    pass->SetStorage(sceneData->camera, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(sceneData->lights, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(sceneData->frame, RenderGraph::STORAGE_WRITE_ONLY);
    pass->Execute([=](const RenderInfo &info) {

        CameraInfo cameraInfo = {};
        cameraInfo.P = scene->camera->GetProjection();
        cameraInfo.V = scene->camera->GetView();
        cameraInfo.invVP = glm::inverse(cameraInfo.P * cameraInfo.V);
        cameraInfo.position = scene->camera->GetPosition();
        cameraInfo.zNear = scene->camera->GetNear();
        cameraInfo.zFar = scene->camera->GetFar();
        cameraInfo.zFarRcp = 1.0f / scene->camera->GetFar();

        glm::vec3 camGridPos = glm::floor(scene->camera->GetPosition() / SURFEL_GRID_SIZE) * SURFEL_GRID_SIZE;

        // transform for non-linear surfel grid transform
        float x = (SURFEL_GRID_INNER_DIMS.x / 2.0) * SURFEL_GRID_SIZE;
        float y = (SURFEL_GRID_INNER_DIMS.y / 2.0) * SURFEL_GRID_SIZE;
        float z = (SURFEL_GRID_INNER_DIMS.z / 2.0) * SURFEL_GRID_SIZE;
        float zFar = cameraInfo.zFar;

        // non-linear transforms for non-linear grid
        cameraInfo.surfelGridFrustumPosX = glm::frustum(-z, z, -y, y, x, zFar)
                                         * glm::lookAt(camGridPos,
                                                       camGridPos
                                                       + glm::vec3(1.0, 0.0, 0.0),
                                                         glm::vec3(0.0, 1.0, 0.0));
        cameraInfo.surfelGridFrustumNegX = glm::frustum(-z, z, -y, y, x, zFar)
                                         * glm::lookAt(camGridPos,
                                                       camGridPos
                                                       + glm::vec3(-1.0, 0.0, 0.0),
                                                         glm::vec3(0.0, 1.0, 0.0));
        cameraInfo.surfelGridFrustumPosZ = glm::frustum(-x, x, -y, y, z, zFar)
                                         * glm::lookAt(camGridPos,
                                                       camGridPos
                                                       + glm::vec3(0.0, 0.0, 1.0),
                                                         glm::vec3(0.0, 1.0, 0.0));
        cameraInfo.surfelGridFrustumNegZ = glm::frustum(-x, x, -y, y, z, zFar)
                                         * glm::lookAt(camGridPos,
                                                       camGridPos
                                                       + glm::vec3(0.0, 0.0, -1.0),
                                                         glm::vec3(0.0, 1.0, 0.0));
        cameraInfo.surfelGridFrustumPosY = glm::frustum(-x, x, -z, z, y, zFar)
                                         * glm::lookAt(camGridPos,
                                                       camGridPos
                                                       + glm::vec3(0.0, 1.0, 0.0),
                                                         glm::vec3(0.0, 0.0, 1.0));
        cameraInfo.surfelGridFrustumNegY = glm::frustum(-x, x, -z, z, y, zFar)
                                         * glm::lookAt(camGridPos,
                                                       camGridPos
                                                       + glm::vec3(0.0, -1.0, 0.0),
                                                         glm::vec3(0.0, 0.0, 1.0));

        FrameInfo frameInfo = {};
        frameInfo.frameID = frameID;
        frameInfo.resolution.x = info.renderFrame->GetExtent().width;
        frameInfo.resolution.y = info.renderFrame->GetExtent().height;

        info.commandBuffer->CopyDataToBuffer(scene->lights, scene->lightBuffer);
        info.commandBuffer->CopyDataToBuffer(cameraInfo, scene->cameraBuffer);
        info.commandBuffer->CopyDataToBuffer(frameInfo, scene->frameInfoBuffer);
    });

    frameID++;
}
