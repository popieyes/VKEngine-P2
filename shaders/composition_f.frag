#version 460

#extension GL_ARB_shader_draw_parameters : enable
#define INV_PI 0.31830988618
#define PI   3.14159265358979323846264338327950288

layout( location = 0 ) in vec2 f_uvs;

//globals
struct LightData
{
    vec4 m_light_pos;
    vec4 m_radiance;
    vec4 m_attenuattion;
    mat4 m_view_projection;
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

layout ( set = 0, binding = 1 ) uniform sampler2D i_albedo;
layout ( set = 0, binding = 2 ) uniform sampler2D i_position_and_depth;
layout ( set = 0, binding = 3 ) uniform sampler2D i_normal;
layout ( set = 0, binding = 4 ) uniform sampler2D i_material;
layout ( set = 0, binding = 5 ) uniform sampler2DArray i_shadow_maps;


layout(location = 0) out vec4 out_color;


float evalVisibility(vec3 frag_pos, vec3 normal, uint light_id) {
    LightData light = per_frame_data.m_lights[light_id];
    uint light_type = uint(floor(light.m_light_pos.a));
    
    // Ambient lights are always visible
    if (light_type == 2) return 1.0;
   
    // Transform fragment position to light space
    vec4 frag_pos_light_space = light.m_view_projection * vec4(frag_pos, 1.0);
    
    // Perspective divide
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    
    // Transform to [0,1] range for texture sampling
    proj_coords = proj_coords * 0.5 + 0.5;
    
    // Check if fragment is outside light frustum
    if (proj_coords.z > 1.0 || 
        proj_coords.x < 0.0 || proj_coords.x > 1.0 || 
        proj_coords.y < 0.0 || proj_coords.y > 1.0) {
        return 0.0;
    }
    
    // Get depth from shadow map
    float closest_depth = texture(i_shadow_maps, vec3(proj_coords.xy, light_id)).r;
    float current_depth = proj_coords.z;
    
    // Basic shadow mapping with simple bias
    //float bias = 0.005;
    
    // Return 1 if visible, 0 if in shadow
    return current_depth <= closest_depth ? 1.0 : 0.0;
}

vec3 evalDiffuse()
{
    vec4  albedo       = texture( i_albedo  , f_uvs );
    vec3  n            = normalize( texture( i_normal, f_uvs ).rgb * 2.0 - 1.0 );    
    vec3  frag_pos     = texture( i_position_and_depth, f_uvs ).xyz;
    vec3  shading = vec3( 0.0 );


    for( uint id_light = 0; id_light < per_frame_data.m_number_of_lights; id_light++ )
    {
        LightData light = per_frame_data.m_lights[ id_light ];
        uint light_type = uint( floor( light.m_light_pos.a ) );

        // Check visibility
        float visibility = evalVisibility(frag_pos, n, id_light);

        switch( light_type )
        {
            case 0: //directional
            {
                vec3 l = normalize(-light.m_light_pos.xyz );
                shading += max( dot( n, l ), 0.0 ) * albedo.rgb * visibility;
                break;
            }
            case 1: //point
            {
                vec3 l = (light.m_light_pos).xyz - frag_pos;
                float dist = length( l );
                float att = 1.0 / (light.m_attenuattion.x + light.m_attenuattion.y * dist + light.m_attenuattion.z * dist * dist );
                vec3 radiance = light.m_radiance.rgb * att;

                shading += max( dot( n, l ), 0.0 ) * albedo.rgb * radiance * visibility;
                break;
            }
            case 2: //ambient
            {
                shading += light.m_radiance.rgb * albedo.rgb;
                break;
            }
        }
    }

    return shading;
}

float normalDistribution(vec3 h, vec3 n, float roughness){
	float alpha = pow(roughness, 2.f);
    float dotNH = max(dot(n, h), 0.0);
	float pow_alpha = pow(alpha, 2.f);
	float ndf = pow_alpha / (PI * pow(pow(dotNH, 2.f) * (pow_alpha - 1) + 1, 2.f));
    return ndf;
}

float geometricTerm(vec3 v, vec3 n, float roughness){
	float k = pow(roughness + 1, 2) / 8;
	float dotNV = max(dot(n, v), 0.0);
	
	float g = dotNV/(dotNV * (1 - k) + k);
	
	return g;
}

float fresnelScalar(vec3 v, vec3 h, float F0){
	float dotVH = max(dot(v, h), 0.0);
	
	float fresnel = F0 + (1 - F0) * pow(2, -5.55573f * dotVH - 6.98316f * dotVH);
	//float fresnel = F0 + (1 - F0) * pow(1 - dotVH, 5);
	return fresnel;
}

vec3 fresnelVec(vec3 v, vec3 h, vec3 F0){
	float dotVH = max(dot(v, h), 0.0);
	
	vec3 fresnel = F0 + (1 - F0) * pow(2, -5.55573f * dotVH - 6.98316f * dotVH);
	//float fresnel = F0 + (1 - F0) * pow(1 - dotVH, 5);
	return fresnel;
}

vec3 evalMicrofacets()
{
    // Retrieve material properties from textures
    vec4 albedo = texture(i_albedo, f_uvs);
    vec3 n = normalize(texture(i_normal, f_uvs).rgb * 2.0 - 1.0);
    vec3 frag_pos = texture(i_position_and_depth, f_uvs).xyz;
    vec4 material_params = texture(i_material, f_uvs);

    float metallic = material_params.r;
    float roughness = material_params.g;
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic); // Interpolate between dielectric and metallic F0
    
    vec3 v = normalize(per_frame_data.m_camera_pos.xyz - frag_pos);
    vec3 shading = vec3(0.0);

    for(uint id_light = 0; id_light < per_frame_data.m_number_of_lights; id_light++)
    {
        LightData light = per_frame_data.m_lights[id_light];
        uint light_type = uint(floor(light.m_light_pos.a));
        
        vec3 l;
        vec3 radiance;
        float visibility = evalVisibility(frag_pos, n, id_light);
        // Calculate light direction and radiance based on light type
        switch(light_type)
        {
            case 0: // directional
                l = normalize(-light.m_light_pos.xyz);
                radiance = light.m_radiance.rgb;
                break;
            case 1: // point
                l = ( per_frame_data.m_inv_view * light.m_light_pos).xyz - frag_pos;
                float dist = length(l);
                l = normalize(l);
                float att = 1.0 / (light.m_attenuattion.x + light.m_attenuattion.y * dist + light.m_attenuattion.z * dist * dist);
                radiance = light.m_radiance.rgb * att;
                break;
            case 2: // ambient
                shading += light.m_radiance.rgb * albedo.rgb;
                continue; // Skip BRDF calculation for ambient
        }
        
        // Calculate half vector
        vec3 h = normalize(v + l);
        
        // Calculate dot products
        float NdotV = max(dot(n, v), 0.0001);
        float NdotL = max(dot(n, l), 0.0001);
        float NdotH = max(dot(n, h), 0.0001);
        float VdotH = max(dot(v, h), 0.0001);
        
        // 1. Normal Distribution Function (GGX/Trowbridge-Reitz)
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
        float D = alpha2 / (PI * denom * denom);
        
        // 2. Geometric term (Schlick)
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        float G1_v = NdotV / (NdotV * (1.0 - k) + k);
        float G1_l = NdotL / (NdotL * (1.0 - k) + k);
        float G = G1_v * G1_l;
        
        // 3. Fresnel term (Modified Schlick)
        float Fc = pow(1.0 - VdotH, 5.0);
        vec3 F = F0 + (1.0 - F0) * pow(2.0, (-5.55473 * VdotH - 6.98316) * VdotH);
        
        // Combine terms for specular BRDF
        vec3 specular = (D * F * G) / (4.0 * NdotV * NdotL);
        
        // Calculate diffuse (Lambert) - only for non-metals
        vec3 diffuse = (1.0 - metallic) * albedo.rgb / PI;
        
        // Combine diffuse and specular
        shading += (diffuse + specular) * radiance * NdotL * visibility;
    }

    return shading;
}

void main() 
{
    float gamma = 2.2f;
    float exposure = 1.0f;
   
    vec3 mapped;

	int id_material = int(texture(i_material, f_uvs).x);
	
	switch(id_material){
		case 0:
			mapped = vec3( 1.0f ) - exp(-evalDiffuse() * exposure);
			break;
		case 1:
			mapped = vec3( 1.0f ) - exp(-evalMicrofacets() * exposure);
			break;
		default:
			mapped = vec3(1.0f);
			break;
	}	

    out_color = vec4( pow( mapped, vec3( 1.0f / gamma ) ), 1.0 );
}