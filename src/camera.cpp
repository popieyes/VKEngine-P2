#include "camera.h"


using namespace MiniEngine;


Camera::Camera( const Runtime& i_runtime ) :
    m_runtime( i_runtime ),
    m_fovy( 1.0f / std::tan( degToRad( 30.f / 2.0f ) ) ),
    m_dirty( { true, true } ),
    m_is_perspective( true ), 
    m_use_target( true ),
    m_position( { 0.f, 0.f, 0.f } ),
    m_up( { 0.f, 1.f, 0.f } ),
    m_target( { 0.f, 0.f, -10.f } ),
    m_near( 0.5f ),
    m_far( 5.f ),
    m_left( -0.5f ),
    m_right( 0.5f ),
    m_bottom( -0.5f ),
    m_top( 0.5f ),
    m_camera_data( {} ),
    m_width( 800 ),
    m_height( 800 )
{

}

Camera::~Camera()
{

}

bool Camera::initialize()
{
    //nothing to do here
    return true;
}


void Camera::shutdown()
{
    //nothing to do here
}


Matrix4f Camera::getView() 
{
    if( m_dirty[ 0 ] )
    {
        if( m_use_target )
        {
            m_camera_data.m_view = glm::lookAt( m_position, m_target, m_up );
        }
        else
        { 
        //glm::vec3 upVector = glm::vec3(0, 1, 0);
        //// rotate around to a given bearing: yaw
        //glm::mat4 camera = glm::rotate(glm::mat4(), bearing, upVector);
        //// Define the 'look up' axis, should be orthogonal to the up axis
        //glm::vec3 pitchVector = glm::vec3(1, 0, 0);
        //// rotate around to the required head tilt: pitch
        //camera = glm::rotate(camera, tilt, pitchVector);
        //// now get the view matrix by taking the camera inverse
        //glm::mat4 view = glm::inverse(camera);
        }

        m_dirty[ 0 ] = false;
    }

    return m_camera_data.m_view;
}


Matrix4f Camera::getProjection() 
{
    if( m_dirty[ 1 ] )
    {
        m_camera_data.m_projection = m_is_perspective ? glm::perspective( m_fovy, static_cast<float>( m_width ) / static_cast<float>( m_height ), m_near, m_far ) : glm::ortho( m_left, m_right, m_bottom, m_top );
        m_dirty[ 1 ] = false;
    }
    return m_camera_data.m_projection;
}


Matrix4f Camera::getViewProjection() 
{
    if( m_dirty[ 0 ] || m_dirty[ 1 ] )
    {
        m_camera_data.m_view_projection = getView() * getProjection();
    }

    return m_camera_data.m_view_projection;
}



std::shared_ptr<Camera> Camera::createCamera( const Runtime& i_runtime, const pugi::xml_node& i_camera_node )
{
    auto camera = std::make_shared<Camera>( i_runtime );

    if( strcmp( i_camera_node.attribute( "type" ).value(), "perspective" ) == 0  )
    {
        camera->m_is_perspective = true;
        camera->m_fovy = i_camera_node.find_child_by_attribute( "name", "fov" ) ? toFloat( i_camera_node.find_child_by_attribute( "name", "fov" ).attribute("value").value() ) : 30.f; 
        camera->m_fovy = degToRad( camera->m_fovy );
    }
    else if( strcmp( i_camera_node.attribute( "type" ).value(), "orthographic" ) == 0  )
    {
        camera->m_is_perspective = false;

        //ortho
        camera->m_left   = i_camera_node.find_child_by_attribute( "name", "left"   ) ? toFloat( i_camera_node.find_child_by_attribute( "name", "left"    ).attribute("value").value() ) : 1.f; 
        camera->m_right  = i_camera_node.find_child_by_attribute( "name", "right"  ) ? toFloat( i_camera_node.find_child_by_attribute( "name", "right"   ).attribute("value").value() ) : 1.f; 
        camera->m_bottom = i_camera_node.find_child_by_attribute( "name", "bottom" ) ? toFloat( i_camera_node.find_child_by_attribute( "name", "bottom"  ).attribute("value").value() ) : 1.f; 
        camera->m_top    = i_camera_node.find_child_by_attribute( "name", "top"    ) ? toFloat( i_camera_node.find_child_by_attribute( "name", "top"     ).attribute("value").value() ) : 1.f;  
    }

    camera->m_use_target = true;

    //transformation
    for (pugi::xml_node node : i_camera_node.children("transform"))
    {
        size_t count = std::distance( node.children("lookat").begin(), node.children("lookat").end() );   

        if( count > 1 )
        {
            throw MiniEngineException( "More than 1 lookAt defined for the camera" );
        }
        
        if( node.child( "lookat" ) )
        {
            uint32 value_counter = 0;
            if( node.child( "lookat" ).attribute( "target" ) )
            {
                camera->m_target = toVector3f( node.child( "lookat" ).attribute( "target" ).value() );
                value_counter++;
            }

            if( node.child( "lookat" ).attribute( "origin" ) )
            {
                camera->m_position = toVector3f( node.child( "lookat" ).attribute( "origin" ).value() );
                value_counter++;
            }

            if( node.child( "lookat" ).attribute( "up" ) )
            {
                camera->m_up = toVector3f( node.child( "lookat" ).attribute( "up" ).value() );
                value_counter++;
            }

            if( value_counter != 3 )
            {
                throw MiniEngineException( "LookAt not fully defined or malformed" );
            }
        }
    }    

    //clipping planes
    if( i_camera_node.child( "clip" )  )
    {
        if( i_camera_node.find_child_by_attribute( "name", "far" ) )
        {
            camera->m_far = toFloat( i_camera_node.find_child_by_attribute ( "name", "far"  ).attribute("value").value() );
        }

        if( i_camera_node.find_child_by_attribute( "name", "near" ) )
        {
            camera->m_far = toFloat( i_camera_node.find_child_by_attribute ( "name", "near"  ).attribute("value").value() );
        }
    }

    //film size
    camera->m_width  = i_camera_node.find_child_by_attribute( "name", "width" ) ? toInt( i_camera_node.find_child_by_attribute ( "name", "width"  ).attribute("value").value() ) : 800; 
    camera->m_height = i_camera_node.find_child_by_attribute( "name", "height" ) ? toInt( i_camera_node.find_child_by_attribute( "name", "height" ).attribute("value").value() ) : 800;

    camera->initialize();

    return camera;

}