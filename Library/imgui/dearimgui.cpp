#include "core/debug.h"
#include "core/window.h"
#include "core/vkutils.h"
#include "core/renderframe.h"
#include "imnodes.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui/dearimgui.h"

using namespace slim;

DearImGui::DearImGui(Context* context) : context(context) {
    InitImGui();
    InitRenderPass();
    InitDescriptorPool();
    InitGlfw();
    InitVulkan();
    InitFontAtlas();
}

DearImGui::~DearImGui() {
    if (renderPass) {
        vkDestroyRenderPass(context->GetDevice(), renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    if (descriptorPool) {
        vkDestroyDescriptorPool(context->GetDevice(), descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
}

void DearImGui::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();
}

void DearImGui::InitDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.poolSizeCount = poolSizes.size();
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = 1000;
    ErrorCheck(vkCreateDescriptorPool(context->GetDevice(), &createInfo, nullptr, &descriptorPool),
            "create a descriptor pool for imgui");
}

void DearImGui::InitRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = context->GetWindow()->GetFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    ErrorCheck(vkCreateRenderPass(context->GetDevice(), &renderPassInfo, nullptr, &renderPass), "create imgui render pass");
}

void DearImGui::InitGlfw() {
    ImGui_ImplGlfw_InitForVulkan(context->GetWindow()->window, true);
}

void DearImGui::InitVulkan() {
    ImGui_ImplVulkan_InitInfo info;
    info.Instance = context->instance;
    info.PhysicalDevice = context->physicalDevice;
    info.Device = context->device;
    info.QueueFamily = context->queueFamilyIndices.graphics.value();
    info.PipelineCache = VK_NULL_HANDLE;
    info.DescriptorPool = descriptorPool;
    info.Subpass = 0;
    info.MinImageCount = context->GetWindow()->swapchainImages.size();
    info.ImageCount = context->GetWindow()->swapchainImages.size();
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.Allocator = nullptr;
    info.CheckVkResultFn = [](VkResult result) {
        if (result != VK_SUCCESS) {
            std::cerr << "[DearImGui Error]: " << result << std::endl;
        }
    };
    ImGui_ImplVulkan_Init(&info, renderPass);
}

void DearImGui::InitFontAtlas() {
    auto frame = SlimPtr<RenderFrame>(context);
    auto commandBuffer = frame->RequestCommandBuffer(VK_QUEUE_TRANSFER_BIT);
    commandBuffer->Begin();
    ImGui_ImplVulkan_CreateFontsTexture(*commandBuffer);
    commandBuffer->End();
    commandBuffer->Submit();
    context->WaitIdle();
}

void DearImGui::Begin() const {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    ImNodes::CreateContext();
}

void DearImGui::End() const {
    ImNodes::DestroyContext();
    ImGui::EndFrame();
}

void DearImGui::Draw(CommandBuffer *commandBuffer) const {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);
}
