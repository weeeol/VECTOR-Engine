#pragma once

#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>

namespace VECTOR {

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanSwapchain {
    public:
        VulkanSwapchain(VulkanContext* context, uint32_t width, uint32_t height);
        ~VulkanSwapchain();

        void Recreate(uint32_t width, uint32_t height);
        
        VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
        VkResult Present(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore);
        
        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        VkExtent2D GetExtent() const { return m_Extent; }
        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        
        size_t GetImageCount() const { return m_Images.size(); }
        VkFramebuffer GetFramebuffer(int index) const { return m_Framebuffers[index]; }

    private:
        void Create();
        void Cleanup();

        void CreateSwapchain();
        void CreateImageViews();
        void CreateRenderPass();
        void CreateDepthResources();
        void CreateFramebuffers();

        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        VkFormat FindDepthFormat();

    private:
        VulkanContext* m_Context;
        uint32_t m_Width, m_Height;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkFormat m_ImageFormat;
        VkExtent2D m_Extent;

        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;

        VkImage m_DepthImage = VK_NULL_HANDLE;
        VmaAllocation m_DepthImageAllocation = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;
        VkFormat m_DepthFormat;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;
    };

} // namespace VECTOR
