#include "core/debug.h"
#include "core/window.h"
#include "core/vkutils.h"

using namespace slim;

WindowDesc& WindowDesc::SetResolution(uint32_t w, uint32_t h) {
    width = w;
    height = h;
    return *this;
}

WindowDesc& WindowDesc::SetFullScreen(bool value) {
    fullscreen = value;
    return *this;
}

WindowDesc& WindowDesc::SetResizable(bool value) {
    resizable = value;
    return *this;
}

WindowDesc& WindowDesc::SetAlphaComposite(bool value) {
    alphaComposite = value;
    return *this;
}

WindowDesc& WindowDesc::SetMaxFramesInFlight(uint32_t value) {
    maxFramesInFlight = value;
    return *this;
}

WindowDesc& WindowDesc::SetMaxSetsPerPool(uint32_t value) {
    maxSetsPerPool = value;
    return *this;
}

WindowDesc& WindowDesc::SetTitle(const std::string &value) {
    title = value;
    return *this;
}

Window::Window(Device *device, const WindowDesc &desc) : desc(desc), device(device) {
    // for Vulkan, use GLFW_NO_API
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // resizable window
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GL_TRUE : GL_FALSE);

    // create a glfw window
    handle = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);

    // user pointer
    glfwSetWindowUserPointer(handle, this);

    // create surface
    ErrorCheck(glfwCreateWindowSurface(device->GetContext()->GetInstance(), handle, nullptr, &surface), "create window surface");

    InitSwapchain();
    InitRenderFrames();
}

Window::~Window() {
    renderFrames.clear();
    inflightFences.clear();

    // clean up image views
    swapchainImages.clear();

    // clean up swapchain
    if (swapchain) {
        vkDestroySwapchainKHR(*device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    // clean up surface
    if (surface) {
        vkDestroySurfaceKHR(device->GetContext()->GetInstance(), surface, nullptr);
        surface = nullptr;
    }

    // clean up window
    if (handle) {
        glfwDestroyWindow(handle);
        handle = nullptr;
    }
}

void Window::PollEvents() {
    glfwPollEvents();
}

bool Window::ShouldClose() {
    return glfwWindowShouldClose(handle);
}

void Window::InitSwapchain() {
    SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device->GetContext()->GetPhysicalDevice(), surface);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(handle, swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilyIndices(device->GetContext()->GetPhysicalDevice(), surface);
    uint32_t queueFamilyIndices[] = { indices.graphics.value(), indices.present.value() };
    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.clipped = VK_TRUE;
    createInfo.presentMode = presentMode;
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = desc.alphaComposite ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
                                                    : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.oldSwapchain = swapchain;

    swapchainFormat = surfaceFormat.format;
    swapchainExtent = extent;

    ErrorCheck(vkCreateSwapchainKHR(*device, &createInfo, nullptr, &swapchain), "create swapchain");

    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, images.data());
    swapchainImages.resize(imageCount);
    for (uint32_t i = 0; i < images.size(); i++) {
        swapchainImages[i] = SlimPtr<GPUImage>(device, swapchainFormat,
                                               VkExtent3D { swapchainExtent.width, swapchainExtent.height, 1 },
                                               1, 1, VK_SAMPLE_COUNT_1_BIT,
                                               images[i]);
    }
}

void Window::InitRenderFrames() {
    maxFramesInFlight = desc.maxFramesInFlight;
    for (uint32_t i = 0; i < desc.maxFramesInFlight; i++) {
        inflightFences.push_back(SlimPtr<Fence>(device));
        renderFrames.push_back(SlimPtr<RenderFrame>(device, desc.maxSetsPerPool));
    }
}

RenderFrame* Window::AcquireNext() {
    RenderFrame *frame = renderFrames.at(currentFrame).get();
    VkFence fence = VK_NULL_HANDLE;

    // reset frame's inflight fence
    if (frame->inflightFence) {
        frame->inflightFence->Wait();
        frame->inflightFence->Reset();
        fence = *frame->inflightFence;
    }

    // reset frame for frame resource recycling
    frame->Reset();

    // update some information needed by frame if swapchain is created
    if (swapchain) {
        uint32_t imageIndex;
        VkSemaphore imageAvailableSemaphore = *frame->imageAvailableSemaphore.get();
        VkResult result = vkAcquireNextImageKHR(*device, swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr, &imageIndex);
        if (result == VK_SUCCESS) {
            frame->swapchain = swapchain;
            frame->swapchainIndex = imageIndex;
            frame->backBuffer = swapchainImages[imageIndex];
            frame->inflightFence = inflightFences[currentFrame].get();
            frame->backBuffer->layouts[0][0] = VK_IMAGE_LAYOUT_UNDEFINED; // reset backbuffer layout
        } else if (result == VK_SUBOPTIMAL_KHR) {
            OnResize();
            return AcquireNext();
        } else {
            throw std::runtime_error("Failed to acquire next image!");
        }
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
    return frame;
}

void Window::OnResize() {
    // get newer window size
    int width = 0, height = 0;
    glfwGetFramebufferSize(handle, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(handle, &width, &height);
        glfwWaitEvents();
    }
    desc.width = width;
    desc.height = height;

    // wait until all things are finished
    device->WaitIdle();

    // recreate most of the rendering related stuff
    InitSwapchain();

    // reset synchronization objects
    for (auto fence : inflightFences) {
        fence->Reset();
    }

    // reset all frames
    for (uint32_t i = 0; i < renderFrames.size(); i++) {
        RenderFrame* frame = renderFrames[i].get();
        frame->Invalidate();
        frame->inflightFence = nullptr;
        frame->imageAvailableSemaphore = SlimPtr<Semaphore>(device);
        frame->renderFinishesSemaphore = SlimPtr<Semaphore>(device);
        frame->SetBackBuffer(swapchainImages[i].get());
    }
}
