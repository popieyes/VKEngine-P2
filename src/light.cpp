#include "light.h"
#include "meshRegistry.h"


using namespace MiniEngine;


std::shared_ptr<Light> Light::createLight(  const Runtime& i_runtime, const pugi::xml_node& emitter )
{
    //for now we convert area lights into pointlights
    auto light = std::make_shared<Light>( i_runtime );

    if( strcmp( emitter.attribute("type").value(), "ambient" ) == 0 )
    {
        light->m_data.m_type = LightType::Ambient;
    }
    else if( strcmp( emitter.attribute("type").value(), "directional" ) == 0 )
    {
        light->m_data.m_type = LightType::Directional;
        auto node = emitter.find_child_by_attribute( "name", "direction" );

        if( !node )
        {
            throw MiniEngineException( "Direction undefinned" );
        }

        light->m_data.m_position = normalize( toVector3f( node.attribute( "value" ).value() ) );
    }
    else if( strcmp( emitter.attribute("type").value(), "point" ) == 0 )
    {
        light->m_data.m_type = LightType::Point;
        auto node = emitter.find_child_by_attribute( "name", "attenuation" );

        if( node )
        {
            light->m_data.m_attenuation = normalize( toVector3f( node.attribute( "value" ).value() ) );
        }

        node = emitter.find_child_by_attribute( "name", "position" );

        if( node )
        {
            light->m_data.m_position = normalize( toVector3f( node.attribute( "value" ).value() ) );
        }
    }

    //radiance and transform
    if( !emitter.find_child_by_attribute( "name", "radiance" ) )
    {
        throw MiniEngineException( "Radiance undefinned" );
    }


    light->m_data.m_radiance = toVector3f( emitter.find_child_by_attribute( "name", "radiance" ).attribute( "value" ).value() );
    light->initialize();

    return light;
}