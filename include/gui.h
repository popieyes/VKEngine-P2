//#pragma once
//
//#include "common.h"
//
//
//namespace MiniEngine
//{
//    class Runtime;
//
//    class Gui
//    {
//    public:
//        Gui ( const Runtime& i_runtime );
//        ~Gui(                          );
//
//        bool initialize();
//        void shutdown  ();
//
//        void update  ();
//        void newFrame();
//        void draw    ();
//
//    private:
//        void initResources();
//
//        const Runtime&                  m_runtime;
//        VkBuffer                        m_vertex_buffer;
//        VkDeviceMemory                  m_vertex_buffer_memory;
//        VkBuffer                        m_index_buffer;
//        VkDeviceMemory                  m_index_buffer_memory;
//        VkImage                         m_font_image;
//        VkDeviceMemory                  m_font_memory;
//        VkImageView                     m_font_image_view;
//        VkSampler                       m_sampler;
//        VkPipelineCache                 m_pipeline_cache;
//        VkPipeline                      m_pipeline;
//        VkRenderPass                    m_render_pass;
//        VkPipelineLayout                m_pipeline_layout;
//        VkDescriptorPool                m_descriptor_pool;
//        VkDescriptorSet                 m_descriptor_set;
//        VkDescriptorSetLayout           m_descriptor_set_layout;
//
//        std::array<VkCommandBuffer, 3>  m_command_buffer;
//
//    };
//}
//
//
