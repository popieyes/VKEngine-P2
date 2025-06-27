#version 460

#extension GL_ARB_shader_draw_parameters : enable

//inputs
layout( location = 0 ) in vec3 v_positions;
layout( location = 1 ) in vec3 v_normals;
layout( location = 2 ) in vec2 v_uvs;


layout( location = 0 ) out vec2 f_uvs;


void main() {
     f_uvs = vec2( v_uvs.x, 1.0 - v_uvs.y );
     gl_Position = vec4( v_positions.x, v_positions.y, 0.0, 1.0 );
}