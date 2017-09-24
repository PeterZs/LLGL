/*
 * VKRenderContext.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "VKRenderContext.h"
#include "VKCommandBuffer.h"
#include "VKCore.h"
#include <LLGL/Platform/NativeHandle.h>
#include "../../Core/Helper.h"
#include <set>


namespace LLGL
{


/* ----- Common ----- */

static const std::vector<const char*> g_deviceExtensions
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VKRenderContext::VKRenderContext(
    const VKPtr<VkInstance>& instance,
    VkPhysicalDevice physicalDevice,
    const VKPtr<VkDevice>& device,
    RenderContextDescriptor desc,
    const std::shared_ptr<Surface>& surface) :
        instance_            { instance                      },
        physicalDevice_      { physicalDevice                },
        device_              { device                        },
        surface_             { instance, vkDestroySurfaceKHR },
        swapChain_           { device, vkDestroySwapchainKHR },
        swapChainRenderPass_ { device, vkDestroyRenderPass   }
{
    SetOrCreateSurface(surface, desc.videoMode, nullptr);
    CreateVkSurface();
    CreateSwapChain(desc.videoMode);
    CreateSwapChainImageViews(device);
    CreateSwapChainRenderPass();
    CreateSwapChainFramebuffers(device);
    CreatePresentSemaphorse();
    AcquireNextPresentImage();
}

void VKRenderContext::Present()
{
    if (!commandBuffer_)
        throw std::runtime_error("no command buffer set to present render context");

    /* End command buffer and render pass */
    commandBuffer_->EndRenderPass();
    commandBuffer_->EndCommandBuffer();

    /* Initialize semaphorse */
    VkSemaphore waitSemaphorse[] = { imageAvailableSemaphore_ };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphorse[] = { renderFinishedSemaphore_ };
    VkCommandBuffer commandBuffers[] = { commandBuffer_->GetBufferObject() };

    /* Submit command buffer to graphics queue */
    VkSubmitInfo submitInfo;
    {
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext                = nullptr;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphorse;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = commandBuffers;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphorse;
    }
    auto result = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    VKThrowIfFailed(result, "failed to submit Vulkan graphics queue");

    /* Present result on screen */
    VkSwapchainKHR swapChains[] = { swapChain_ };

    VkPresentInfoKHR presentInfo;
    {
        presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext               = nullptr;
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = signalSemaphorse;
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = swapChains;
        presentInfo.pImageIndices       = &presentImageIndex_;
        presentInfo.pResults            = nullptr;
    }
    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    VKThrowIfFailed(result, "failed to present Vulkan graphics queue");

    /* Get image index for next presentation */
    AcquireNextPresentImage();
}

/* ----- Configuration ----- */

void VKRenderContext::SetVideoMode(const VideoModeDescriptor& videoModeDesc)
{
    //todo
}

void VKRenderContext::SetVsync(const VsyncDescriptor& vsyncDesc)
{
    //todo
}

/* --- Extended functions --- */

void VKRenderContext::SetPresentCommandBuffer(VKCommandBuffer* commandBuffer)
{
    commandBuffer_ = commandBuffer;
    commandBuffer_->SetPresentIndex(presentImageIndex_);
}


/*
 * ======= Private: =======
 */

void VKRenderContext::CreateVkSurface()
{
    /* Get hantive handle from context surface */
    NativeHandle nativeHandle;
    GetSurface().GetNativeHandle(&nativeHandle);

    #if defined LLGL_OS_WIN32

    /* Setup Win32 surface descriptor */
    VkWin32SurfaceCreateInfoKHR surfaceDesc;
    {
        surfaceDesc.sType       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceDesc.pNext       = nullptr;
        surfaceDesc.flags       = 0;
        surfaceDesc.hinstance   = GetModuleHandle(NULL);
        surfaceDesc.hwnd        = nativeHandle.window;
    }
    auto result = vkCreateWin32SurfaceKHR(instance_, &surfaceDesc, nullptr, surface_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Win32 surface for Vulkan render context");

    #endif
}

void VKRenderContext::CreateSwapChain(const VideoModeDescriptor& desc)
{
    /* Query swap-chain support for physics device and surface  */
    auto swapChainSupport = VKQuerySwapChainSupport(physicalDevice_, surface_);

    /* Find queue families */
    auto queueFamilyIndices = VKFindQueueFamilies(physicalDevice_, VK_QUEUE_GRAPHICS_BIT);

    /* Get device queues for graphics and presentation */
    vkGetDeviceQueue(device_, queueFamilyIndices.graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilyIndices.presentFamily, 0, &presentQueue_);

    /* Pick surface format, present mode, and extent */
    auto surfaceFormat  = PickSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode    = PickSwapPresentMode(swapChainSupport.presentModes);
    auto extent         = PickSwapExtent(swapChainSupport.caps, static_cast<uint32_t>(desc.resolution.x), static_cast<uint32_t>(desc.resolution.y));

    /* Determine required image count for swap-chain */
    uint32_t imageCount = swapChainSupport.caps.minImageCount;
    if (swapChainSupport.caps.maxImageCount > 0)
        imageCount = std::min(imageCount, swapChainSupport.caps.maxImageCount);

    /* Create swap-chain */
    VkSwapchainCreateInfoKHR createInfo;
    {
        createInfo.sType                        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext                        = nullptr;
        createInfo.flags                        = 0;
        createInfo.surface                      = surface_;
        createInfo.minImageCount                = imageCount;
        createInfo.imageFormat                  = surfaceFormat.format;
        createInfo.imageColorSpace              = surfaceFormat.colorSpace;
        createInfo.imageExtent                  = extent;
        createInfo.imageArrayLayers             = 1;
        createInfo.imageUsage                   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
        if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
        {
            createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount    = 2;
            createInfo.pQueueFamilyIndices      = queueFamilyIndices.indices;
        }
        else
        {
            createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount    = 0;
            createInfo.pQueueFamilyIndices      = nullptr;
        }

        createInfo.preTransform                 = swapChainSupport.caps.currentTransform;
        createInfo.compositeAlpha               = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode                  = presentMode;
        createInfo.clipped                      = VK_TRUE;
        createInfo.oldSwapchain                 = VK_NULL_HANDLE;
    }
    auto result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, swapChain_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan swap-chain");

    /* Query swap-chain images */
    result = vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
    VKThrowIfFailed(result, "failed to query number of Vulkan swap-chain images");

    swapChainImages_.resize(imageCount);
    result = vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());
    VKThrowIfFailed(result, "failed to query Vulkan swap-chain images");

    /* Store swap-chain format and extent */
    swapChainFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;
}

void VKRenderContext::CreateSwapChainImageViews(const VKPtr<VkDevice>& device)
{
    swapChainImageViews_.resize(swapChainImages_.size(), VKPtr<VkImageView>{ device, vkDestroyImageView });

    for (std::size_t i = 0, n = swapChainImages_.size(); i < n; ++i)
    {
        /* Create image view */
        VkImageViewCreateInfo createInfo;
        {
            createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.pNext                            = nullptr;
            createInfo.flags                            = 0;
            createInfo.image                            = swapChainImages_[i];
            createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                           = swapChainFormat_;
            createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel    = 0;
            createInfo.subresourceRange.levelCount      = 1;
            createInfo.subresourceRange.baseArrayLayer  = 0;
            createInfo.subresourceRange.layerCount      = 1;
        }
        auto result = vkCreateImageView(device_, &createInfo, nullptr, swapChainImageViews_[i].ReleaseAndGetAddressOf());
        VKThrowIfFailed(result, "failed to create Vulkan swap-chain image view");
    }
}

void VKRenderContext::CreateSwapChainRenderPass()
{
    /* Initialize attachment descriptor */
    VkAttachmentDescription attachmentDesc;
    {
        attachmentDesc.flags                = 0;
        attachmentDesc.format               = swapChainFormat_;
        attachmentDesc.samples              = VK_SAMPLE_COUNT_1_BIT;
        attachmentDesc.loadOp               = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDesc.storeOp              = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDesc.finalLayout          = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    /* Initialize attachment reference */
    VkAttachmentReference attachmentRef;
    {
        attachmentRef.attachment            = 0;
        attachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    /* Initialize sub-pass descriptor */
    VkSubpassDescription subpassDesc;
    InitMemory(subpassDesc);
    {
        subpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.colorAttachmentCount    = 1;
        subpassDesc.pColorAttachments       = (&attachmentRef);
    }

    /* Initialize sub-pass dependency */
    VkSubpassDependency subpassDep;
    {
        subpassDep.srcSubpass               = VK_SUBPASS_EXTERNAL;
        subpassDep.dstSubpass               = 0;
        subpassDep.srcStageMask             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDep.dstStageMask             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDep.srcAccessMask            = 0;
        subpassDep.dstAccessMask            = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDep.dependencyFlags          = 0;
    }

    /* Create swap-chain render pass */
    VkRenderPassCreateInfo createInfo;
    {
        createInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.pNext                    = nullptr;
        createInfo.flags                    = 0;
        createInfo.attachmentCount          = 1;
        createInfo.pAttachments             = (&attachmentDesc);
        createInfo.subpassCount             = 1;
        createInfo.pSubpasses               = (&subpassDesc);
        createInfo.dependencyCount          = 1;
        createInfo.pDependencies            = (&subpassDep);
    }
    auto result = vkCreateRenderPass(device_, &createInfo, nullptr, swapChainRenderPass_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan swap-chain render pass");
}

void VKRenderContext::CreateSwapChainFramebuffers(const VKPtr<VkDevice>& device)
{
    swapChainFramebuffers_.resize(swapChainImageViews_.size(), VKPtr<VkFramebuffer>{ device, vkDestroyFramebuffer });

    for (std::size_t i = 0, n = swapChainImageViews_.size(); i < n; ++i)
    {
        /* Create framebuffer */
        VkFramebufferCreateInfo createInfo;
        {
            createInfo.sType            = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.pNext            = nullptr;
            createInfo.flags            = 0;
            createInfo.renderPass       = swapChainRenderPass_;
            createInfo.attachmentCount  = 1;
            createInfo.pAttachments     = &(swapChainImageViews_[i]);
            createInfo.width            = swapChainExtent_.width;
            createInfo.height           = swapChainExtent_.height;
            createInfo.layers           = 1;
        }
        auto result = vkCreateFramebuffer(device_, &createInfo, nullptr, swapChainFramebuffers_[i].ReleaseAndGetAddressOf());
        VKThrowIfFailed(result, "failed to create Vulkan swap-chain framebuffer");
    }
}

void VKRenderContext::CreateVkSemaphore(VKPtr<VkSemaphore>& semaphore)
{
    /* Create semaphore (no flags) */
    VkSemaphoreCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
    }
    auto result = vkCreateSemaphore(device_, &createInfo, nullptr, semaphore.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan semaphore");
}

void VKRenderContext::CreatePresentSemaphorse()
{
    /* Create presentation semaphorse */
    CreateVkSemaphore(imageAvailableSemaphore_);
    CreateVkSemaphore(renderFinishedSemaphore_);
}

VkSurfaceFormatKHR VKRenderContext::PickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats) const
{
    if (surfaceFormats.empty())
        throw std::runtime_error("no Vulkan surface formats available");

    if (surfaceFormats.size() == 1 && surfaceFormats.front().format == VK_FORMAT_UNDEFINED)
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    for (const auto& format : surfaceFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return surfaceFormats.front();
}

VkPresentModeKHR VKRenderContext::PickSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
{
    for (const auto& mode : presentModes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VKRenderContext::PickSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCaps, uint32_t width, uint32_t height) const
{
    if (surfaceCaps.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
        return VkExtent2D
        {
            std::max(surfaceCaps.minImageExtent.width, std::min(surfaceCaps.maxImageExtent.width, width)),
            std::max(surfaceCaps.minImageExtent.height, std::min(surfaceCaps.maxImageExtent.height, height))
        };
    }
    return surfaceCaps.currentExtent;
}

void VKRenderContext::AcquireNextPresentImage()
{
    /* Get next image for presentation */
    vkAcquireNextImageKHR(device_, swapChain_, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore_, VK_NULL_HANDLE, &presentImageIndex_);
}


} // /namespace LLGL



// ================================================================================