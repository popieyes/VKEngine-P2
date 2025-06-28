#version 460

#extension GL_EXT_ray_query : enable
#extension GL_ARB_shader_draw_parameters : enable
#define INV_PI 0.31830988618
#define PI   3.14159265358979323846264338327950288
#define RTX

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
layout(set = 0, binding = 6) uniform accelerationStructureEXT TLAS;

layout(location = 0) out vec4 out_color;


vec3 sampleDirectionInCone(vec3 coneDirection, float coneAngle, uint seed) {
    // Método de muestreo uniforme en el cono
    float u1 = fract(sin(dot(vec2(seed, seed + 1), vec2(12.9898, 78.233))) * 43758.5453);
    float u2 = fract(sin(dot(vec2(seed + 2, seed + 3), vec2(39.3468, 11.1357))) * 24634.6345);

    float cosTheta = mix(cos(coneAngle), 1.0, pow(u1, 4.0));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * 3.141592 * u2;

    vec3 direction;
    direction.x = cos(phi) * sinTheta;
    direction.y = sin(phi) * sinTheta;
    direction.z = cosTheta;

    // Crear base ortonormal para transformar la dirección
    vec3 up = abs(coneDirection.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, coneDirection));
    vec3 bitangent = cross(coneDirection, tangent);

    return normalize(tangent * direction.x + bitangent * direction.y + coneDirection * direction.z);
}

//Ray Tracing defines
#define NUM_SOFT_SHADOW_RAYS 16
#define LIGHT_RADIUS 0.5   

// Ray Tracing Visibility Evaluation with Soft Shadows
float evalVisibility(vec3 frag_pos, vec3 normal, vec3 light_dir, float coneAngle, int numSamples) {
    // Origen del rayo
    vec3 origin = frag_pos + normal * 0.01;

    // Distancia mínima y máxima de recorrido del rayo
    float t_min = 0.001;
    float t_max = 100.0;

    int visibleCount = 0;
    uint randSeed = uint(gl_FragCoord.x * 17.0 + gl_FragCoord.y * 131.0);

    for (int i = 0; i < numSamples; ++i) {
        // Dirección del rayo
        vec3 sample_dir = sampleDirectionInCone(light_dir, coneAngle, randSeed + uint(i));

        // Inicialización del ray query
        rayQueryEXT ray_query;

        rayQueryInitializeEXT(
            ray_query,
            TLAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
            0xFF,
            origin,
            t_min,
            sample_dir,
            t_max
        );

        // Busqueda de colisiones
        while(rayQueryProceedEXT(ray_query)) {
            if(rayQueryGetIntersectionTypeEXT(ray_query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                rayQueryConfirmIntersectionEXT(ray_query);
            }
        }

        if (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            visibleCount += 1;
        }
    }

    // Se calcula la visibilidad como el porcentaje de rayos no bloqueados
    float rawVisibility = float(visibleCount) / float(numSamples);

    return clamp((rawVisibility - 0.2) / 0.8, 0.0, 1.0); // Se ajusta el valor para suavizar el umbral de sombra (entre 0 y 1)
}

// Ray Tracing Visibility Evaluation
float evalVisibility(vec3 frag_pos, vec3 normal, vec3 light_dir) {
    // Origen del rayo
    vec3 origin = frag_pos + normal * 0.01;

    // Dirección del rayo
    vec3 direction = normalize(light_dir);

    // Distancia mínima y máxima de recorrido del rayo
    float t_min = 0.001;
    float t_max = 100.0;

    // Inicialización del ray query
    rayQueryEXT ray_query;

    rayQueryInitializeEXT(
        ray_query,
        TLAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        origin,
        t_min,
        direction,
        t_max
    );

    // Busqueda de colisiones
    bool hit = false;
    while(rayQueryProceedEXT(ray_query)) {
        if(rayQueryGetIntersectionTypeEXT(ray_query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
            rayQueryConfirmIntersectionEXT(ray_query);
        }
    }

     if (rayQueryGetIntersectionTypeEXT(ray_query, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
        hit = true;
    } 

    return hit ? 0.0 : 1.0;
}

// Shadow Mapping Visibility Evaluation
float evalVisibility(vec3 frag_pos, uint id_light, vec3 normal) {
    LightData light = per_frame_data.m_lights[id_light];
    vec4 light_space_pos = light.m_view_projection * vec4(frag_pos, 1.0);
    
    vec3 projCoords = light_space_pos.xyz / light_space_pos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    
  /*   // PCF 
    vec2 texel_size = 1.0 / vec2(textureSize(i_shadow_maps, 0));
    float shadow = 0.0;
    
    // Kernel de 3x3 con muestreo estratificado
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            // Muestreo con patrón de rotación para reducir artifacts
            float rot_angle = float(id_light) * 0.785; // Rotación única por luz
            mat2 rot = mat2(cos(rot_angle), -sin(rot_angle), 
                          sin(rot_angle),  cos(rot_angle));
            vec2 offset = rot * vec2(x, y) * texel_size;
            
            float closest_depth = texture(i_shadow_maps, vec3(projCoords.xy + offset, float(id_light))).r;
            shadow += (currentDepth > closest_depth) ? 0.0 : 1.0;
        }
    }
    
    // Promedio de muestras
    shadow /= 9.0; */
    
    // Basic algorithm for shadow mapping
    float sampleDepth = texture(i_shadow_maps, vec3(projCoords.xy, float(id_light))).r;
    float shadow = (sampleDepth < currentDepth) ? 0.0 : 1.0;
    
    return shadow;
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
        float visibility = 1.0f;

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
                visibility = evalVisibility(frag_pos, id_light,n);
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
        float visibility = 1.0f;
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
                //visibility = evalVisibility(frag_pos  ,n,l);
                float lightRadius = 0.025;
                float coneAngle = atan(lightRadius / dist);
                int numSamples = 64;
               /*  // Soft Shadows
                visibility = evalVisibility(frag_pos, n, (light.m_light_pos).xyz - frag_pos,coneAngle, numSamples);
                // Hard Shadows
                visibility = evalVisibility(frag_pos, n, (light.m_light_pos).xyz - frag_pos); */
                // Shadow Mapping
                 visibility = evalVisibility(frag_pos, id_light, n); 
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