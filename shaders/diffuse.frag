#version 460

#extension GL_ARB_shader_draw_parameters : enable

layout( location = 0 ) in vec3 f_position;
layout( location = 1 ) in vec3 f_normal;
layout( location = 2 ) in vec2 f_uv;
layout( location = 3 ) in flat int  f_instance;



//globals
struct LightData
{
    vec4 m_light_pos;
    vec4 m_radiance;
    vec4 m_attenuattion;
};

layout( std140, set = 0, binding = 0 ) uniform PerFrameData
{
    vec4      m_camera_pos;
    mat4      m_view;
    mat4      m_projection;
    mat4      m_view_projection;
    mat4      m_inv_view;
    mat4      m_inv_projection;
    mat4      m_inv_view_projection;
    vec4      m_clipping_planes;
    LightData m_lights[ 10 ];
    uint      m_number_of_lights;
} per_frame_data;


struct ObjectData
{
    mat4 m_model;
    vec4 m_albedo; 
    vec4 m_metallic_roughness;
};

//all object matrices
layout(std140,set = 1, binding = 0) readonly buffer ObjectBufferData
{
    ObjectData objects[];
} per_object_data;


layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_position_depth;
layout(location = 3) out vec4 out_material;


float linearDepth( float depth )
{
    float z = depth * 2.0f - 1.0f; 
    return ( 2.0f * per_frame_data.m_clipping_planes.x * per_frame_data.m_clipping_planes.y ) / (per_frame_data.m_clipping_planes.y + per_frame_data.m_clipping_planes.x  - z * (per_frame_data.m_clipping_planes.y - per_frame_data.m_clipping_planes.x )); 
}


void main() {
    out_color           = per_object_data.objects[ f_instance ].m_albedo;
    out_normal          = vec4( normalize( f_normal ) * 0.5f + 0.5f, 0.0f );
    out_position_depth  = vec4( f_position, linearDepth(gl_FragCoord.z) );
    out_material        = vec4( 0.0, 0.0, 0.0, 1.0 ); //0 for diffuse
}