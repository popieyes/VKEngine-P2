#include "entity.h"
#include "meshRegistry.h"
#include "material.h"
#include "light.h"
#include "camera.h"
#include "runtime.h"
#include "frame.h"
#include "vulkan/meshVK.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/utilsVK.h"
#include "vulkan/windowVK.h"

using namespace MiniEngine;


std::shared_ptr<Entity> Entity::createEntity(  const Runtime& i_runtime, const pugi::xml_node& i_node, const uint32_t i_id )
{
    if( !i_node.child( "bsdf" ) || !i_node.find_child_by_attribute( "name", "filename" ) )
    {
        throw MiniEngineException( "BSDF or Obj no defined" );
    }

    auto entity = std::make_shared<Entity>( i_runtime );
    std::string path = i_node.find_child_by_attribute( "name", "filename" ).attribute("value").value();
    
    entity->m_mesh = i_runtime.m_mesh_registry->loadMesh( path );

    for (pugi::xml_node node = i_node.child("transform"); node; node = node.next_sibling("transform") )
    {
        if( node.child( "lookat" ) )
        {
            throw MiniEngineException( "Mesh transform node cannot have lookat" );
        }

        entity->m_transform = entity->m_transform * Transform::createTransform( node );
    }

    entity->m_material = Material::createMaterial( i_runtime, i_node.child( "bsdf" ) );

    assert( entity != nullptr );

    //load material 
    entity->initialize();
    entity->m_entity_offset = i_id;
    
    return  entity;
}

bool Entity::initialize()
{
    assert( m_material  != nullptr );
    assert( m_mesh      != nullptr );

    return true;
}

void Entity::shutdown()
{
}


void Entity::draw( CommandBuffer& i_command_buffer, const Frame& i_frame )
{
    //make the draw
    const RendererVK& renderer = *m_runtime.m_renderer;
    
    m_mesh->draw( i_command_buffer, m_entity_offset );
}




