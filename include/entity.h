#pragma once

#include "common.h"
#include "transform.h"


typedef VkCommandBuffer CommandBuffer;

namespace MiniEngine
{
    class MeshVK;
    class Material;
    struct Frame;
    struct Runtime;

    class Entity final 
    {
    public:
        explicit Entity( const Runtime& i_runtime ) : m_runtime( i_runtime ){};
        ~Entity        () = default;
    
        bool initialize();

        void shutdown();

        static std::shared_ptr<Entity> createEntity(  const Runtime& i_runtime, const pugi::xml_node& i_node, const uint32_t i_id );

        void draw( CommandBuffer& i_command_buffer,  const Frame& i_frame );

        inline Transform& getTransform()
       {
           return m_transform;
       }

       inline Material& getMaterial() const
       {
           return *m_material;
       }

       inline uint32_t getEntityOffset() const
       {
           return m_entity_offset;
       }

    private:
        Entity( const Entity& ) = delete;
        Entity& operator=(const Entity& ) = delete;

        const Runtime& m_runtime;
        std::shared_ptr<MeshVK> m_mesh;
        Transform m_transform;
        std::shared_ptr<Material> m_material;

        uint32_t m_entity_offset;
    };
};