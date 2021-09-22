#include "overlay.h"

void AddOverlayPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    RenderGraph::Resource* colorBuffer,
                    GBuffer* gbuffer,
                    DearImGui* ui) {

    RenderFrame* frame = renderGraph.GetRenderFrame();

    // resources
    // NOTE: no additional resources created

    // compile
    auto overlayPass = renderGraph.CreateRenderPass("overlay");
    overlayPass->SetColor(colorBuffer);
    overlayPass->SetTexture(gbuffer->albedoBuffer);
    overlayPass->SetTexture(gbuffer->normalBuffer);
    overlayPass->SetTexture(gbuffer->positionBuffer);
    overlayPass->SetTexture(gbuffer->depthBuffer);

    // execute
    overlayPass->Execute([=](const RenderInfo& info) {
        ImTextureID albedo   = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->albedoBuffer->GetImage()->AsTexture());
        ImTextureID normal   = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->normalBuffer->GetImage()->AsTexture());
        ImTextureID position = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->positionBuffer->GetImage()->AsTexture());
        ImTextureID depth    = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->depthBuffer->GetImage()->AsDepthBuffer());
        ui->Begin();
        {
            ImVec2 winsize = ImVec2(200, 500);
            ImGui::SetNextWindowSize(winsize, ImGuiCond_Once);
            ImGui::Begin("GBuffer");
            {
                ImVec2 region = ImGui::GetContentRegionAvail();
                ImVec2 size = ImVec2(region.x, region.x / frame->GetAspectRatio());
                ImGui::Image(albedo, size);
                ImGui::Image(normal, size);
                ImGui::Image(position, size);
                ImGui::Image(depth, size);
            }
            ImGui::End();
        }
        ui->End();
        ui->Draw(info.commandBuffer);
    });
}
