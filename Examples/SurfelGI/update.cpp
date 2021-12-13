#include "update.h"
#include "exposure.h"

void AddUpdatePass(RenderGraph&       graph,
                   AutoReleasePool&   pool,
                   render::GBuffer*   gbuffer,
                   render::SceneData* sceneData,
                   render::Surfel*    surfel,
                   render::Debug*     debug,
                   Scene*             scene) {

    static uint32_t frameID = 0;

    auto pass = graph.CreateComputePass("update");
    pass->SetStorage(sceneData->camera, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(sceneData->lights, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(sceneData->sky, RenderGraph::STORAGE_WRITE_ONLY);
    pass->SetStorage(sceneData->frame, RenderGraph::STORAGE_WRITE_ONLY);
    pass->Execute([=](const RenderInfo &info) {

        // camera
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

        // frame
        FrameInfo frameInfo = {};
        frameInfo.frameID = frameID;
        frameInfo.resolution.x = info.renderFrame->GetExtent().width;
        frameInfo.resolution.y = info.renderFrame->GetExtent().height;

        // pre-compute exposure in CPU
        // float ev100 = EV100(16, 0.008, 100);
        float ev100 = 10;
        float exposure = Exposure(ev100);

        // light transforms
        std::vector<glm::mat4> lightXform;
        for (const auto& light : scene->lights) {
            glm::mat4 xform(1.0);
            xform = glm::translate(xform, light.position);  // apply translation
            xform = glm::scale(xform, glm::vec3(0.1));      // apply scale
            lightXform.push_back(xform);
        }

        // light info
        std::vector<LightInfo> lightInfos;
        for (const auto& light : scene->lights) {
            lightInfos.push_back(light);
            // translate point light intensity into luminous power
            lightInfos.back().intensity *= (4 * M_PI);
            // pre-apply exposure to intensity
            lightInfos.back().intensity *= exposure;
        }

        // sky info
        SkyInfo skyInfo = {};
        skyInfo.color = scene->sky.color;
        skyInfo.intensity = scene->sky.intensity * exposure;

        // bilateral info
        std::vector<BilateralInfo> bilaterals = {};
        {
            BilateralInfo data = {};
            data.singleStepOffset = glm::vec2(1.0 / info.renderFrame->GetExtent().width, 0.0);
            data.distanceNormalizationFactor = 8.0;
            bilaterals.push_back(data);
        }
        {
            BilateralInfo data = {};
            data.singleStepOffset = glm::vec2(0.0, 1.0 / info.renderFrame->GetExtent().height);
            data.distanceNormalizationFactor = 8.0;
            bilaterals.push_back(data);
        }

        info.commandBuffer->CopyDataToBuffer(lightXform, scene->lightXformBuffer);
        info.commandBuffer->CopyDataToBuffer(lightInfos, scene->lightBuffer);
        info.commandBuffer->CopyDataToBuffer(cameraInfo, scene->cameraBuffer);
        info.commandBuffer->CopyDataToBuffer(frameInfo,  scene->frameInfoBuffer);
        info.commandBuffer->CopyDataToBuffer(skyInfo,    scene->skyBuffer);
        info.commandBuffer->CopyDataToBuffer(bilaterals, scene->bilateralBuffer);
    });

    frameID++;
}
