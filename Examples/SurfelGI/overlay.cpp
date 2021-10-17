#include "config.h"
#include "scene.h"
#include "overlay.h"

void BuildOverlayUI(DearImGui *ui) {
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ui->Begin();
    {
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize   |
                                      ImGuiWindowFlags_NoMove     |
                                      ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoTitleBar);
        {
            ImGuiID dockspaceId = ImGui::GetID("Main");
            ImGui::DockSpace(dockspaceId, ImVec2(0.0, 0.0), dockspaceFlags);
            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, ImVec2(960, 720));

            // split into 2 nodes
            auto leftDockId = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, nullptr, &dockspaceId);

            ImGui::DockBuilderDockWindow("Debug", leftDockId);
            ImGui::DockBuilderFinish(dockspaceId);
        }
        ImGui::End();
    }
    ui->End();
}

void AddOverlayPass(RenderGraph&           graph,
                    AutoReleasePool&       pool,
                    render::GBuffer*       gbuffer,
                    render::SceneData*     sceneData,
                    render::Surfel*        surfel,
                    render::Debug*         debug,
                    Scene*                 scene,
                    DearImGui*             ui,
                    RenderGraph::Resource* colorAttachment) {

    auto overlayPass = graph.CreateRenderPass("overlay");
    overlayPass->SetColor(colorAttachment, ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    #ifdef ENABLE_GBUFFER_VISUALIZATION
    overlayPass->SetTexture(gbuffer->albedo);
    overlayPass->SetTexture(gbuffer->normal);
    overlayPass->SetTexture(debug->depth);
    #endif
    #ifdef ENABLE_OBJECT_VISUALIZATION
    overlayPass->SetTexture(debug->object);
    #endif
    #ifdef ENABLE_GRID_VISUALIZATION
    overlayPass->SetTexture(debug->surfelGrid);
    #endif
    overlayPass->Execute([=](const RenderInfo &info) {
        ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

        #ifdef ENABLE_GBUFFER_VISUALIZATION
        ImTextureID albedo = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->albedo->GetImage()->AsTexture());
        ImTextureID normal = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->normal->GetImage()->AsTexture());
        ImTextureID depth  = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->depth->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_OBJECT_VISUALIZATION
        ImTextureID object = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->object->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_GRID_VISUALIZATION
        ImTextureID grid   = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->surfelGrid->GetImage()->AsTexture());
        #endif

        ui->Begin();
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
            ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize   |
                                          ImGuiWindowFlags_NoMove     |
                                          ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoTitleBar);
            {
                ImGuiID dockspaceId = ImGui::GetID("Main");
                ImGui::DockSpace(dockspaceId, ImVec2(0.0, 0.0), dockspaceFlags);
            }
            ImGui::End();

            ImGui::Begin("Debug");
            {
                ImVec2 region = ImGui::GetContentRegionAvail();
                ImVec2 size = ImVec2(region.x, region.x / info.renderFrame->GetAspectRatio());

                ImGui::BeginTabBar("##GBuffer");
                {

                    #ifdef ENABLE_GBUFFER_VISUALIZATION
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
                    #endif

                    #ifdef ENABLE_OBJECT_VISUALIZATION
                    if (ImGui::BeginTabItem("Object")) {
                        ImGui::Image(object, size);
                        ImGui::EndTabItem();
                    }
                    #endif

                    #ifdef ENABLE_GRID_VISUALIZATION
                    if (ImGui::BeginTabItem("Grid")) {
                        ImGui::Image(grid, size);
                        ImGui::EndTabItem();
                    }
                    #endif
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }
        ui->End();
        ui->Draw(info.commandBuffer);
    });
}
