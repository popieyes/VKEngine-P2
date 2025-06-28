#pragma once

#include "vulkan/renderPassVK.h"

namespace MiniEngine
{
    struct Runtime;
    class Entity;
    typedef std::shared_ptr<Entity> EntityPtr;

    class blurPassVK final : public RenderPassVK
    {
    public:
        blurPassVK(
            const Runtime& i_runtime,
            const ImageBlock& i_ssao_attachment);
        virtual ~blurPassVK();

        bool            initialize() override;
        void            shutdown() override;
        VkCommandBuffer draw(const Frame& i_frame) override;

        void addEntityToDraw(const EntityPtr i_entity) override;

        void createBlur();

        const ImageBlock& getBlurOutput() const { return m_blurOutput; }

    private:
        blurPassVK(const blurPassVK&) = delete;
        blurPassVK& operator=(const blurPassVK&) = delete;

        void createFbo();
        void createRenderPass();
        void createPipelines();
        void createDescriptorLayout();
        void createDescriptors();


        struct DescriptorsSets
        {
            VkDescriptorSet m_per_frame_descriptor;
        };

        struct MaterialPipeline
        {
            // prepare the different render supported depending on the material
            VkPipeline                                                         m_pipeline;
            VkPipelineLayout                                                   m_pipeline_layouts;
            std::array<VkDescriptorSetLayout, 2                    > m_descriptor_set_layout; //2 sets, per frame and per object
            std::array<DescriptorsSets, 3                    > m_descriptor_sets;
            std::array<VkPipelineShaderStageCreateInfo, 2                    > m_shader_stages;
        };

        std::array<MaterialPipeline, 2> m_pipelines; //one by material

        VkRenderPass                   m_render_pass;
        std::array<VkCommandBuffer, 3> m_command_buffer;
        std::array<VkFramebuffer, 3> m_fbos;
        VkDescriptorPool               m_descriptor_pool;

        std::unordered_map<uint32_t, std::vector<EntityPtr>> m_entities_to_draw;

        const ImageBlock m_ssao_attachment;
        ImageBlock m_blurOutput;
        VkSampler m_linearSampler;
    };
};