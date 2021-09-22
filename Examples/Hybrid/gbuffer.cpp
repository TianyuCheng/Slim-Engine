#include "gbuffer.h"

void AddGBufferPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    Camera* camera,
                    GBuffer* gbuffer,
                    scene::Node* scene) {

    // compile
    auto gbufferPass = renderGraph.CreateRenderPass("gbuffer");
    gbufferPass->SetColor(gbuffer->albedoBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    gbufferPass->SetColor(gbuffer->normalBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    gbufferPass->SetColor(gbuffer->positionBuffer, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    gbufferPass->SetDepthStencil(gbuffer->depthBuffer, ClearValue(1.0f, 0));

    // execute
    gbufferPass->Execute([=](const RenderInfo& info) {
        // sceneFilter result + sorting
        auto culling = CPUCulling();
        culling.Cull(scene, camera);
        culling.Sort(RenderQueue::Geometry,    RenderQueue::GeometryLast, SortingOrder::FrontToback);
        culling.Sort(RenderQueue::Transparent, RenderQueue::Transparent,  SortingOrder::BackToFront);

        // rendering
        MeshRenderer renderer(info);
        renderer.Draw(camera, culling.GetDrawables(RenderQueue::Geometry, RenderQueue::GeometryLast));
    });
}
