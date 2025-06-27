#pragma once

#include "common.h"

namespace MiniEngine
{
    struct LightData
    {
        alignas( 16 ) Vector4f m_light_pos;
        alignas( 16 ) Vector4f m_radiance;
        alignas( 16 ) Vector4f m_attenuattion;
    };

    struct PerFrameData
    {
        alignas( 16 ) Vector4f m_camera_pos;
        alignas( 16 ) Matrix4f m_view;
        alignas( 16 ) Matrix4f m_projection;
        alignas( 16 ) Matrix4f m_view_projection;
        alignas( 16 ) Matrix4f m_inv_view;
        alignas( 16 ) Matrix4f m_inv_projection;
        alignas( 16 ) Matrix4f m_inv_view_projection;
        alignas( 16 ) Vector4f m_clipping_planes;
        //light info
        alignas( 16 ) LightData m_lights[ kMAX_NUMBER_LIGHTS ];
        alignas( 4  ) uint32_t  m_number_of_lights;
    };

    struct PerObjectData
    {
        //for now we only have material data
        alignas( 16 ) Matrix4f m_model;
        alignas( 16 ) Vector4f m_albedo; 
        alignas( 16 ) Vector4f m_metallic_roughness;
    };

    struct Frame
    {};
};