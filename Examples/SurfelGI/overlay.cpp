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
    overlayPass->SetColor(colorAttachment);

    #ifdef ENABLE_GBUFFER_VISUALIZATION
    overlayPass->SetTexture(gbuffer->albedo);
    overlayPass->SetTexture(gbuffer->normal);
    overlayPass->SetTexture(debug->depth);
    #endif

    #ifdef ENABLE_OBJECT_VISUALIZATION
    overlayPass->SetTexture(debug->object);
    #endif

    #ifdef ENABLE_SURFEL_GRID_VISUALIZATION
    overlayPass->SetTexture(debug->surfelGrid);
    #endif

    #ifdef ENABLE_SURFEL_DEBUG_VISUALIZATION
    overlayPass->SetTexture(debug->surfelDebug);
    #endif

    #ifdef ENABLE_SURFEL_COVERAGE_VISUALIZATION
    overlayPass->SetTexture(surfel->surfelCoverage);
    #endif

    #ifdef ENABLE_SURFEL_VARIANCE_VISUALIZATION
    overlayPass->SetTexture(debug->surfelVariance);
    #endif

    #ifdef ENABLE_SURFEL_BUDGET_VISUALIZATION
    overlayPass->SetTexture(debug->surfelBudget);
    #endif

    overlayPass->SetTexture(surfel->surfelDiffuse);

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

        #ifdef ENABLE_SURFEL_GRID_VISUALIZATION
        ImTextureID grid   = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->surfelGrid->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_SURFEL_DEBUG_VISUALIZATION
        ImTextureID debugcov = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->surfelDebug->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_SURFEL_COVERAGE_VISUALIZATION
        ImTextureID coverage = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), surfel->surfelCoverage->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_SURFEL_BUDGET_VISUALIZATION
        ImTextureID alloc    = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->surfelBudget->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_SURFEL_VARIANCE_VISUALIZATION
        ImTextureID variance = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->surfelVariance->GetImage()->AsTexture());
        #endif

        ImTextureID diffuse  = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), surfel->surfelDiffuse->GetImage()->AsTexture());

        ui->Begin();
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0);
            ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize   |
                                          ImGuiWindowFlags_NoMove     |
                                          ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoTitleBar);
            {
                ImGuiID dockspaceId = ImGui::GetID("Main");
                ImGui::DockSpace(dockspaceId, ImVec2(0.0, 0.0), dockspaceFlags);
            }
            ImGui::End();

            ImGui::SetNextWindowBgAlpha(0.5);
            ImGui::Begin("Debug");
            {
                ImVec2 region = ImGui::GetContentRegionAvail();
                ImVec2 size = ImVec2(region.x, region.x / info.renderFrame->GetAspectRatio());
                ImVec2 barSize = ImVec2(size.x, 10);

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

                    #ifdef ENABLE_SURFEL_GRID_VISUALIZATION
                    if (ImGui::BeginTabItem("Grid")) {
                        ImGui::Image(grid, size);
                        ImGui::EndTabItem();
                    }
                    #endif

                    #ifdef ENABLE_SURFEL_DEBUG_VISUALIZATION
                    if (ImGui::BeginTabItem("Debug")) {
                        ImGui::Image(debugcov, size);
                        ImGui::EndTabItem();
                    }
                    #endif

                    #ifdef ENABLE_SURFEL_COVERAGE_VISUALIZATION
                    if (ImGui::BeginTabItem("Coverage")) {
                        ImGui::Image(coverage, size);
                        ImGui::EndTabItem();
                    }
                    #endif

                    #ifdef ENABLE_SURFEL_VARIANCE_VISUALIZATION
                    if (ImGui::BeginTabItem("Variance")) {
                        ImGui::Image(variance, size);
                        ImGui::EndTabItem();
                    }
                    #endif

                    if (ImGui::BeginTabItem("Diffuse")) {
                        ImGui::Image(diffuse, size);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();

                #ifdef ENABLE_SURFEL_BUDGET_VISUALIZATION
                ImGui::Separator();
                ImGui::Image(alloc, barSize);
                #endif

                ImGui::Separator();

                // controller for camera walk speed
                if (ImGui::InputFloat("Walk Speed", &scene->walkSpeed)) {
                    scene->camera->SetWalkSpeed(scene->walkSpeed);
                }

                // controller for surfel configurations
                if (ImGui::Button("Reset Surfels")) {
                    scene->ResetSurfels();
                }
                if (ImGui::Button("Pause Surfels")) {
                    scene->PauseSurfels();
                }
                if (ImGui::Button("Resume Surfels")) {
                    scene->ResumeSurfels();
                }

                // controller for light configurations
                ImGui::BeginTabBar("##Lights");
                for (uint i = 0; i < scene->lights.size(); i++) {
                    std::string tabname = "Light " + std::to_string(i);
                    std::string color = "Color##Light-" + std::to_string(i);
                    std::string position = "Position##Light-" + std::to_string(i);
                    std::string range = "Range##Light-" + std::to_string(i);
                    std::string intensity = "Intensity##Light-" + std::to_string(i);
                    if (ImGui::BeginTabItem(tabname.c_str())) {
                        float _position[3] = {
                            scene->lights[i].position.x,
                            scene->lights[i].position.y,
                            scene->lights[i].position.z,
                        };
                        float _color[3] = {
                            scene->lights[i].color.x,
                            scene->lights[i].color.y,
                            scene->lights[i].color.z,
                        };
                        float _intensity = scene->lights[i].intensity;
                        float _range = scene->lights[i].range;
                        if (ImGui::DragFloat3(position.c_str(), _position, 0.1f, -30.0f, 30.0f)) {
                            scene->lights[i].position.x = _position[0];
                            scene->lights[i].position.y = _position[1];
                            scene->lights[i].position.z = _position[2];
                        }
                        if (ImGui::DragFloat3(color.c_str(), _color, 0.05f, 0.0f, 1.0f)) {
                            scene->lights[i].color.x = _color[0];
                            scene->lights[i].color.y = _color[1];
                            scene->lights[i].color.z = _color[2];
                        }
                        if (ImGui::DragFloat(intensity.c_str(), &_intensity, 1.0f, 0.0f, 100.0f)) {
                            scene->lights[i].intensity = _intensity;
                        }
                        if (ImGui::DragFloat(range.c_str(), &_range, 1.0f, 0.0f, 100.0f)) {
                            scene->lights[i].range = _range;
                        }
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();

            }
            ImGui::End();
        }
        ui->End();
        ui->Draw(info.commandBuffer);
    });
}
