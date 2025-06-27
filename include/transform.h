#pragma once

#include "common.h"

namespace MiniEngine
{
    struct RotAxis 
    {
        Vector3f m_axis;
        float m_angle;
    };

    class Transform final
    {
    public:
        explicit Transform();
        explicit Transform( const Matrix4f& i_mat );
        ~Transform() = default;

        bool initialize();
        void shutdown  ();

        Matrix4f getTransform();
        Matrix4f getInverseTransform() ;

        void translate( const Vector3f& i_translation );
        void rotate   ( const RotAxis&  i_rotation    );
        void scale    ( const Vector3f& i_scale       );

        /// Concatenate with another transform
        Transform operator*(const Transform &t) const;

        static Transform createTransform( const pugi::xml_node& i_node );

    private:
        bool m_dirty;
        Matrix4f m_transform_matrix;
        Matrix4f m_inverse_transform;
    };
};