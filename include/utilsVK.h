#pragma once

#include "common.h"
#include "vulkan/extensionsVK.h"

namespace MiniEngine
{
    class DeviceVK;

    namespace UtilsVK
    {
        void setupCallbacks( DeviceVK& i_device );

        void setObjectName( VkDevice i_device, uint64_t i_object, VkDebugReportObjectTypeEXT i_object_type, const char *i_name );

        void setObjectTag(VkDevice i_device, uint64_t i_object, VkDebugReportObjectTypeEXT i_object_type, uint64_t i_name, size_t i_tag_size, const void* i_tag);

        void beginRegion(VkCommandBuffer i_cmd_buffer, const char* i_pmarker_name, glm::vec4 i_color);

	    void insert(VkCommandBuffer i_cmd_buffer, std::string i_marker_name, glm::vec4 i_color);

        void endRegion(VkCommandBuffer i_cmd_buffer);

        bool getSupportedDepthFormat( VkPhysicalDevice i_physical_device, VkFormat* i_depth_format );

        VkShaderModule loadShader( const std::string i_filename, const VkDevice i_device );

        void createBuffer( const DeviceVK& i_device, VkDeviceSize i_size, VkBufferUsageFlags i_usage, VkMemoryPropertyFlags i_properties, VkBuffer& o_buffer, VkDeviceMemory& o_buffer_memory );

        void copyBuffer( const DeviceVK& i_device, VkBuffer i_src_buffer, VkBuffer i_dst_buffer, VkDeviceSize i_size );
        
        void setImageLayout( VkCommandBuffer i_cmd_buffer, VkImage i_image, VkImageLayout i_old_image_layout, VkImageLayout i_new_image_layout, VkImageSubresourceRange i_subresource_range, VkPipelineStageFlags isrc_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags i_dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
   
        void createImage( const DeviceVK& i_device, VkFormat i_format, VkImageUsageFlagBits i_usage_bits, uint32_t i_width, uint32_t i_height, ImageBlock& o_image_block );
        
        void createImage( const DeviceVK& i_device, VkFormat i_format, VkImageUsageFlagBits i_usage_bits, uint32_t i_width, uint32_t i_height, uint32_t i_depth,uint32_t i_mip_levels, ImageBlockType i_image_type, ImageBlock& o_image_block );
    
        void freeImageBlock( const DeviceVK& i_device, ImageBlock& io_free_image_block );

        void TextureFromBuffer( const DeviceVK& device,void* i_buffer, VkDeviceSize i_buffer_size, VkFormat i_format, uint32_t i_tex_width, uint32_t i_tex_height, ImageBlock& o_new_image, VkFilter i_filter = VK_FILTER_LINEAR, VkImageUsageFlags i_image_usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout i_image_layout= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
   
        VkCommandBuffer initOneTimeCommandBuffer( const DeviceVK& device );
        void            endOneTimeCommandBuffer ( const DeviceVK& device, VkCommandBuffer io_command_buffer );
        
        void createBLAS( const DeviceVK &i_device, VkBuffer i_vertex_buffer, VkBuffer i_index_buffer, const std::vector<Vertex>& m_vertices, 
            const std::vector<uint32_t>& i_indices, VkAccelerationStructureKHR& o_blas, VkBuffer& o_buffer, VkDeviceMemory& o_memory );

        void createTLAS( const DeviceVK &i_device, std::vector<Matrix4f>& i_transforms, std::vector<VkAccelerationStructureKHR>& i_blas_instances,
             VkAccelerationStructureKHR& o_tlas, VkBuffer& o_buffer, VkDeviceMemory& o_memory );

        uint64_t get_device_address( VkDevice i_device, VkBuffer i_buffer );
    };
};
