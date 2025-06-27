#include "transform.h"


using namespace MiniEngine;

Transform::Transform() : 
    m_dirty( true ),
    m_transform_matrix( Matrix4f( 1.f ) ),
    m_inverse_transform( Matrix4f( 1.f ) )
{
}

Transform::Transform( const Matrix4f& i_mat )
{
    m_transform_matrix = i_mat;
    m_inverse_transform = m_transform_matrix;
    m_dirty = false;
}

bool Transform::initialize()
{
    return true;
}


void Transform::shutdown()
{
}


Matrix4f Transform::getTransform() 
{
    return m_transform_matrix;
}


Matrix4f Transform::getInverseTransform() 
{
    if( m_dirty )
    {
        m_inverse_transform = glm::inverse( m_transform_matrix );
        m_dirty = false;
    }

    return m_inverse_transform;
}

void Transform::translate( const Vector3f& i_translation )
{
    m_dirty = true;
    m_transform_matrix = glm::translate( m_transform_matrix, i_translation );
}
        

void Transform::rotate( const RotAxis& i_rotation )
{
    m_dirty = true;
    m_transform_matrix = glm::rotate( m_transform_matrix, i_rotation.m_angle, i_rotation.m_axis );
}
        

void Transform::scale( const Vector3f& i_scale )
{
    m_dirty = true;
    m_transform_matrix = glm::scale( m_transform_matrix, i_scale );
}


/// Concatenate with another transform
Transform Transform::operator*(const Transform &t) const
{
    Transform new_transform( this->m_transform_matrix * t.m_transform_matrix );
    
    return new_transform;
}


Transform Transform::createTransform( const pugi::xml_node& i_node )
{
    Transform transform;


    for( pugi::xml_node_iterator it = i_node.begin(); it != i_node.end(); ++it )
    {
        if( strcmp( it->name(), "scale" ) == 0 )
        {
            if( it->attribute( "value" ) )
            {
                transform.scale( toVector3f( it->attribute( "value" ).value() ) ); 
            }
            else
            {
                throw MiniEngineException( "Transform scale malformed" );
            }
        }

        if( strcmp( it->name(), "rotate" ) == 0 )
        {
            if( it->attribute( "value" ) )
            {
                RotAxis rot_axis;
                rot_axis.m_angle = toFloat( it->attribute( "angle" ).value() );
                rot_axis.m_axis  = toVector3f( it->attribute( "axis" ).value() );
                transform.rotate( rot_axis ); 
            }
            else
            {
                throw MiniEngineException( "Transform scale malformed" );
            }
        }

        if( strcmp( it->name(), "translate" ) == 0 )
        {
            if( it->attribute( "value" ) )
            {
                transform.translate( toVector3f( it->attribute( "value" ).value() ) ); 
            }
            else
            {
                throw MiniEngineException( "Transform translation malformed" );
            }
        }
    }



    transform.initialize();
    return transform;
}