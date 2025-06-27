#pragma once

#include "common.h"


namespace MiniEngine
{
    struct Runtime;

    class MeshVK final
    {
    public:
        explicit MeshVK( const Runtime& i_runtime, const std::string& i_path, const std::vector<uint32_t> i_indices, const std::vector<Vertex> i_vertices );
        ~MeshVK() = default;
    
        bool initialize();
        void shutdown();

        void draw( VkCommandBuffer& i_command_buffer, const uint32_t i_instance_id );

    private:
        MeshVK( const MeshVK& ) = delete;
        MeshVK& operator=(const MeshVK& ) = delete;

        VkBuffer createVertexBuffer( const std::vector<Vertex>& i_data, VkDeviceMemory& i_memory );
        void createIndexBuffer ();

        const Runtime& m_runtime;

        std::string m_path;

        std::vector<uint32_t> m_indices;
        std::vector<Vertex>   m_vertices;

        VkBuffer                                       m_indices_buffer;
        VkBuffer                                       m_data_buffer;
        VkDeviceMemory                                 m_indices_memory;
        VkDeviceMemory                                 m_data_memory;
    
    };
};