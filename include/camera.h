#pragma once

#include "common.h"

namespace MiniEngine
{
    struct Runtime;

    class Camera final
    {
    public:
        struct CameraData
        {
            Matrix4f m_view;
            Matrix4f m_projection;
            Matrix4f m_view_projection;
        };

        Camera ( const Runtime& i_runtime );
        virtual ~Camera();

        bool initialize ();
        void shutdown   ();

        void setFovy( const float i_fovy )
        {
            m_dirty[ 1 ] = true;
            m_fovy  = i_fovy;
        }

        void setFrustum( const float i_left, const float i_right, const float i_bottom, const float i_top )
        {
            m_dirty[ 1 ] = true;
            m_left   = i_left;
            m_right  = i_right;
            m_bottom = i_bottom;
            m_top    = i_top;
        }

        void setClippingPlanes( const float i_near, const float i_far )
        {
            m_dirty[ 1 ] = true;
            m_near = i_near;
            m_far  = i_far;
        }

        void setPosition( const Vector3f& i_position  )
        {
            m_dirty[ 0 ] = true;
            m_position = i_position;
        }

        void setTarget( const Vector3f& i_target, bool i_use_target = true )
        {
            m_dirty[ 0 ] = m_dirty[ 1 ] = m_dirty[ 2 ] = true;
            m_use_target = i_use_target;
            m_target     = i_target;
        }

        inline Vector3f getCameraPos() const
        {
            return m_position;
        }

        Matrix4f getView();

        Matrix4f getProjection();

        Matrix4f getViewProjection();

        inline uint32_t getWidth() const
        {
            return m_width;
        }

        inline uint32_t getHeight() const
        {
            return m_height;
        }

        inline const CameraData& getCameraData()
        {
            if( m_dirty[ 0 ] || m_dirty[ 1 ] )
            {
                getViewProjection();
            }

            return m_camera_data;
        }

        inline float getNearPlane() const
        {
            return m_near;
        }

        inline float getFarPlane() const
        {
            return m_far;
        }

        static std::shared_ptr<Camera> createCamera( const Runtime& i_runtime, const pugi::xml_node& i_camera_node );

    private:
        Camera( const Camera& ) = delete;
        Camera& operator=(const Camera& ) = delete;


        const Runtime&      m_runtime;
        std::array<bool, 2> m_dirty;
        bool                m_is_perspective;
        bool                m_use_target;
        Vector3f            m_position;
        Vector3f            m_up;
        Vector3f            m_target;
        float               m_near;
        float               m_far;
        float               m_fovy;
        float               m_left;
        float               m_right;
        float               m_bottom;
        float               m_top;
        uint32              m_width;
        uint32_t            m_height;
        CameraData          m_camera_data;
    };
};