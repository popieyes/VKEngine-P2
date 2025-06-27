#include "microfacets.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/windowVK.h"
#include "vulkan/utilsVK.h"
#include "runtime.h"


using namespace MiniEngine;


bool Microfacets::initialize() 
{
    const RendererVK& renderer = (const RendererVK&)m_runtime.m_renderer;

    return true;
}


std::shared_ptr<Microfacets> Microfacets::createMaterial( const Runtime& i_runtime, const pugi::xml_node& i_node, bool i_is_dielectric  )
{
    std::shared_ptr<Microfacets> microfacets = std::make_shared<Microfacets>( i_runtime );

    if( i_node.find_child_by_attribute( "name", "roughness" ) )
    {
        microfacets->m_data.m_roughness = toFloat( i_node.find_child_by_attribute( "name", "roughness" ).attribute( "value" ).value() );
    }

    if( i_node.find_child_by_attribute( "name", "metallic" ) )
    {
        microfacets->m_data.m_metallic = toFloat( i_node.find_child_by_attribute( "name", "metallic" ).attribute( "value" ).value() );
    }
    
    if( i_node.find_child_by_attribute( "name", "albedo" ) )
    {
        microfacets->m_data.m_albedo = toVector3f( i_node.find_child_by_attribute( "name", "albedo" ).attribute( "value" ).value() );
    }

    return microfacets;
}