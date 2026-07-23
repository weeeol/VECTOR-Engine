#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "Engine/Graphics/Vulkan/VulkanShader.hpp"

namespace VECTOR {

    struct PipelineConfigInfo {
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
        bool emptyVertexInput = false;
    };

    class VulkanPipeline {
    public:
        VulkanPipeline(const std::string& vertFile, const std::string& fragFile, const PipelineConfigInfo& configInfo);
        ~VulkanPipeline();

        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        void Bind(VkCommandBuffer commandBuffer);
        VkPipelineLayout GetLayout() const { return m_PipelineLayout; }
        
        static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

    private:
        void CreateGraphicsPipeline(const std::string& vertFile, const std::string& fragFile, const PipelineConfigInfo& configInfo);

        VkPipeline m_GraphicsPipeline;
        VkPipelineLayout m_PipelineLayout;
        std::unique_ptr<VulkanShader> m_Shader;
    };

} // namespace VECTOR
