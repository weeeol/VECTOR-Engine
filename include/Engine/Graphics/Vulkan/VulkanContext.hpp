#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <vector>
#include <optional>
#include <mutex>

namespace VECTOR {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    class VulkanContext {
    public:
        VulkanContext();
        ~VulkanContext();

        static VulkanContext* Get() { return s_Instance; }

        bool Initialize(SDL_Window* window);
        void Shutdown();

        VkInstance GetInstance() const { return m_Instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice GetDevice() const { return m_Device; }
        VkSurfaceKHR GetSurface() const { return m_Surface; }
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        uint32_t GetGraphicsQueueFamilyIndex() const { return m_GraphicsFamilyIndex; }
        VmaAllocator GetAllocator() const { return m_Allocator; }
        std::mutex& GetGraphicsQueueMutex() { return m_GraphicsQueueMutex; }

    private:
        bool CreateInstance(SDL_Window* window);
        bool SetupDebugMessenger();
        bool CreateSurface(SDL_Window* window);
        bool PickPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateAllocator();

        bool CheckValidationLayerSupport();
        std::vector<const char*> GetRequiredExtensions(SDL_Window* window);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        bool IsDeviceSuitable(VkPhysicalDevice device);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

    private:
        static VulkanContext* s_Instance;

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        std::mutex m_GraphicsQueueMutex;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        uint32_t m_GraphicsFamilyIndex = 0;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;

        const std::vector<const char*> m_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

#ifdef NDEBUG
        const bool m_EnableValidationLayers = false;
#else
        const bool m_EnableValidationLayers = true;
#endif
    };

} // namespace VECTOR
