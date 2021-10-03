#include "overlay.h"

void AddOverlayPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    RenderGraph::Resource* colorBuffer,
                    GBuffer* gbuffer,
                    Visualize* visualize,
                    SurfelManager* surfel,
                    DearImGui* ui) {

    RenderFrame* frame = renderGraph.GetRenderFrame();

    // resources
    // NOTE: no additional resources created

    // compile
    auto overlayPass = renderGraph.CreateRenderPass("overlay");
    overlayPass->SetColor(colorBuffer);
    overlayPass->SetTexture(gbuffer->albedoBuffer);
    overlayPass->SetTexture(gbuffer->normalBuffer);
    overlayPass->SetTexture(visualize->depthBuffer);
    overlayPass->SetTexture(visualize->objectBuffer);
    overlayPass->SetTexture(visualize->surfelCovBuffer);
    overlayPass->SetTexture(visualize->surfelAllocBuffer);

    // execute
    overlayPass->Execute([=](const RenderInfo& info) {
        ImTextureID albedo      = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->albedoBuffer->GetImage()->AsTexture());
        ImTextureID normal      = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->normalBuffer->GetImage()->AsTexture());
        ImTextureID depth       = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), visualize->depthBuffer->GetImage()->AsTexture());
        ImTextureID object      = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), visualize->objectBuffer->GetImage()->AsTexture());
        ImTextureID surfelCov   = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), visualize->surfelCovBuffer->GetImage()->AsTexture());
        ImTextureID surfelAlloc = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), visualize->surfelAllocBuffer->GetImage()->AsTexture());
        ui->Begin();
        {
            ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
            ImGui::Begin("GBuffer");
            {
                ImVec2 region = ImGui::GetContentRegionAvail();
                ImVec2 size = ImVec2(region.x, region.x / frame->GetAspectRatio());

                ImGui::BeginTabBar("##GBuffer");
                {

                    if (ImGui::BeginTabItem("Albedo")) {
                        ImGui::Image(albedo, size);
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Normal")) {
                        ImGui::Image(normal, size);
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Depth")) {
                        ImGui::Image(depth, size);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
            ImGui::End();

            ImGui::SetNextWindowSize(ImVec2(200, 230), ImGuiCond_Once);
            ImGui::Begin("Surfel");
            {
                ImVec2 region = ImGui::GetContentRegionAvail();
                ImVec2 size = ImVec2(region.x, region.x / frame->GetAspectRatio());
                ImVec2 barSize = ImVec2(size.x, 10);

                ImGui::BeginTabBar("##Surfel");
                {
                    if (ImGui::BeginTabItem("Coverage")) {
                        ImGui::Image(surfelCov, size);
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Object")) {
                        ImGui::Image(object, size);
                        ImGui::EndTabItem();
                    }

                    ImGui::Image(surfelAlloc, barSize);
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }
        ui->End();
        ui->Draw(info.commandBuffer);
    });
}
