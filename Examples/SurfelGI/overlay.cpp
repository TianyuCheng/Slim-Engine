#include <glm/gtc/type_ptr.hpp>
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

    #ifdef ENABLE_SURFEL_COVERAGE_VISUALIZATION
    overlayPass->SetTexture(surfel->surfelCoverage);
    #endif

    #ifdef ENABLE_SURFEL_VARIANCE_VISUALIZATION
    overlayPass->SetTexture(debug->surfelVariance);
    #endif

    #ifdef ENABLE_SURFEL_DEPTH_VISUALIZATION
    overlayPass->SetTexture(surfel->surfelDepth);
    #endif

    #ifdef ENABLE_SURFEL_BUDGET_VISUALIZATION
    overlayPass->SetTexture(debug->surfelBudget);
    #endif

    overlayPass->SetTexture(gbuffer->diffuse);

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

        #ifdef ENABLE_SURFEL_COVERAGE_VISUALIZATION
        ImTextureID coverage = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), surfel->surfelCoverage->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_SURFEL_BUDGET_VISUALIZATION
        ImTextureID alloc    = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->surfelBudget->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_SURFEL_VARIANCE_VISUALIZATION
        ImTextureID variance = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), debug->surfelVariance->GetImage()->AsTexture());
        #endif

        #ifdef ENABLE_SURFEL_DEPTH_VISUALIZATION
        ImTextureID surfelDepth = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), surfel->surfelDepth->GetImage()->AsTexture());
        #endif

        ImTextureID diffuse  = slim::imgui::AddTexture(info.renderFrame->GetDescriptorPool(), gbuffer->diffuse->GetImage()->AsTexture());

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

                    #ifdef ENABLE_SURFEL_DEPTH_VISUALIZATION
                    if (ImGui::BeginTabItem("Radial Depth")) {
                        ImGui::Image(surfelDepth, size);
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
                    info.commandBuffer->GetDevice()->WaitIdle();
                    scene->ResetSurfels();
                }
                if (ImGui::Button("Pause Surfels")) {
                    scene->PauseSurfels();
                }
                if (ImGui::Button("Resume Surfels")) {
                    scene->ResumeSurfels();
                }
                if (ImGui::Button("Toggle Surfel Point")) {
                    scene->surfelDebugControl.debugPoint = !scene->surfelDebugControl.debugPoint;
                }
                if (ImGui::Button("Toggle Light Control")) {
                    scene->lightDebugControl.debugLight = !scene->lightDebugControl.debugLight;
                }

                // controller for light configurations
                uint32_t selectedLight = 0;
                ImGui::BeginTabBar("##Lights");
                for (uint i = 0; i < scene->lights.size(); i++) {
                    auto& light = scene->lights[i];
                    std::string tabname = "Light " + std::to_string(i);
                    std::string color = "Color##Light-" + std::to_string(i);
                    std::string position = "Position##Light-" + std::to_string(i);
                    std::string range = "Range##Light-" + std::to_string(i);
                    std::string intensity = "Intensity##Light-" + std::to_string(i);
                    if (ImGui::BeginTabItem(tabname.c_str())) {
                        ImGui::DragFloat3(position.c_str(), glm::value_ptr(light.position), 0.1f, -30.0f, 30.0f);
                        ImGui::DragFloat3(color.c_str(), glm::value_ptr(light.color), 0.05f, 0.0f, 1.0f);
                        ImGui::DragFloat(intensity.c_str(), &light.intensity, 1.0f, 0.0f, 100.0f);
                        ImGui::DragFloat(range.c_str(), &light.range, 1.0f, 0.0f, 100.0f);
                        if (ImGui::Button("Move Light to Camera")) {
                            light.position = scene->camera->GetPosition();
                        }
                        ImGui::EndTabItem();
                        selectedLight = i;
                    }
                }
                ImGui::EndTabBar();

                // imguizmo
                if (scene->lightDebugControl.debugLight) {
                    ImGuizmo::Enable(true);
                    ImGui::Begin("Main", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
                    {
                        ImGuizmo::SetDrawlist();
                        float x = ImGui::GetWindowPos().x;
                        float y = ImGui::GetWindowPos().y;
                        float w = static_cast<float>(ImGui::GetWindowWidth());
                        float h = static_cast<float>(ImGui::GetWindowHeight());
                        ImGuizmo::SetRect(x, y, w, h);

                        glm::mat4 xform(1.0);
                        auto& light = scene->lights[selectedLight];
                        xform = glm::translate(xform, light.position);  // apply translation
                        ImGuizmo::Manipulate(glm::value_ptr(scene->camera->GetView()),
                                glm::value_ptr(scene->camera->GetProjection()),
                                ImGuizmo::TRANSLATE,
                                ImGuizmo::LOCAL,
                                glm::value_ptr(xform));
                        light.position.x = xform[3][0];
                        light.position.y = xform[3][1];
                        light.position.z = xform[3][2];
                    }
                    ImGui::End();
                }
            }
            ImGui::End();
        }
        ui->End();
        ui->Draw(info.commandBuffer);
    });
}
