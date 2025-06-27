#include "material.h"
#include "diffuse.h"
#include "microfacets.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "runtime.h"

using namespace MiniEngine;



std::shared_ptr<Material> Material::createMaterial( const Runtime& i_runtime, const pugi::xml_node& i_node )
{
    std::shared_ptr<Material> material;

    if( strcmp( i_node.attribute( "type" ).value(), "diffuse" ) == 0 )
    {
        material = Diffuse::createMaterial( i_runtime, i_node );
    }

    if( strcmp( i_node.attribute( "type" ).value(), "microfacet" ) == 0 )
    {
        material = Microfacets::createMaterial( i_runtime, i_node );
    }

    material->initialize();

    return material;
}
