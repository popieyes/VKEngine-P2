#include "diffuse.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/windowVK.h"
#include "vulkan/utilsVK.h"
#include "runtime.h"


using namespace MiniEngine;


bool Diffuse::initialize() 
{
    const RendererVK& renderer = *m_runtime.m_renderer;

    return true;
}


std::shared_ptr<Diffuse> Diffuse::createMaterial( const Runtime& i_runtime, const pugi::xml_node& i_node )
{
    std::shared_ptr<Diffuse> diffuse = std::make_shared<Diffuse>( i_runtime );

    if( i_node.find_child_by_attribute( "name", "albedo" ) )
    {
        diffuse->m_data.m_albedo = toVector3f( i_node.find_child_by_attribute( "name", "albedo" ).attribute( "value" ).value() );
    } 

    return diffuse;
}