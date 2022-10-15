#version 450
#include "Common.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 1) uniform sampler2D DepthTextureSampler;
layout(set = 0, binding = 2) uniform sampler2D NoiseSampler;

layout(location = 0) out vec4 outFrag;

layout(set = 0, binding = 0) uniform UniformBuffer
{
	 mat4 invProj;
	float near;
	float far;
	float p0;
	float p1;
} ubo;

float readDepth( in vec2 coord ) 
{
	return texture( DepthTextureSampler, coord ).x;
	vec2 cameraRange = vec2(0.01, 1000.0);
	return (2.0 * cameraRange.x) / (cameraRange.y + cameraRange.x - texture( DepthTextureSampler, coord ).x * (cameraRange.y - cameraRange.x));	
}

 vec3 reconstruct_position(in vec2 uv, in float z, in mat4 inverse_view_projection)
{
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	vec4 position_s = vec4(x, y, z, 1);
	vec4 position_v = inverse_view_projection * position_s;
	return position_v.xyz / position_v.w;
}

vec3 normalFromDepth(in float depth, in vec2 texCoords)
{
	const vec2 offset1 = vec2(0.0, 0.001);
	const vec2 offset2 = vec2(0.001, 0.0);
	
	float depth1 =  readDepth(texCoords + offset1);
	float depth2 =  readDepth(texCoords + offset2);
	
	vec3 p1 = vec3(offset1, depth1 - depth);
	vec3 p2 = vec3(offset2, depth2 - depth);
	
	vec3 normal = cross(p1, p2);
	normal.z = -normal.z;
	
	return normalize(normal);
}


vec3 reflection(vec3 v1,vec3 v2)
{
    vec3 result = 2.0 * dot(v2, v1) * v2;
    result = v1 - result;
    return result;
}

const float near = 0.1f;
const float far = 1000.f;
float linearDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return depth;
    return (2.0 * near * far) / (far + near - z * (far - near));
}


void main()
{ 
    const float total_strength = 1.0;
    const float base = 0.2;
    
    const float area = 0.0075;
    const float falloff = 0.000001;
    
    const float radius = 0.0002;
    
    const int samples = 16;
    vec3 sample_sphere[samples] = {
        vec3( 0.5381, 0.1856,-0.4319), vec3( 0.1379, 0.2486, 0.4430),
        vec3( 0.3371, 0.5679,-0.0057), vec3(-0.6999,-0.0451,-0.0019),
        vec3( 0.0689,-0.1598,-0.8547), vec3( 0.0560, 0.0069,-0.1843),
        vec3(-0.0146, 0.1402, 0.0762), vec3( 0.0100,-0.1924,-0.0344),
        vec3(-0.3577,-0.5301,-0.4358), vec3(-0.3169, 0.1063, 0.0158),
        vec3( 0.0103,-0.5869, 0.0046), vec3(-0.0897,-0.4940, 0.3287),
        vec3( 0.7119,-0.0154,-0.0918), vec3(-0.0533, 0.0596,-0.5411),
        vec3( 0.0352,-0.0631, 0.5460), vec3(-0.4776, 0.2847,-0.0271)
    };
    
    float depth = readDepth(outTexCoord); 
	
    vec3 random = normalize( texture(NoiseSampler, outTexCoord * 4.0).rgb );
	vec3 position = reconstruct_position(outTexCoord, depth, ubo.invProj); 
	
	
    //vec3 position = vec3(outTexCoord, depth);
    vec3 normal = normalFromDepth(depth, outTexCoord);
	normal = clamp(normal, 0.0, 1.0);
	
    float radius_depth = radius/depth;
    float occlusion = 0.0;
    for(int i=0; i < samples; i++) 
    {
        vec3 ray = radius_depth * reflection(sample_sphere[i], random);
        vec3 hemi_ray = position + sign(dot(ray,normal)) * ray;
        
        float occ_depth = readDepth(saturate(hemi_ray.xy));
        float difference = depth - occ_depth;
        
        occlusion += step(falloff, difference) * (1.0-smoothstep(falloff, area, difference));
    }
    
    float ao = 1.0 - total_strength * occlusion * (1.0 / samples);
	float final = clamp(ao + base,0.0,1.0);
	vec4 ssao = vec4(final,final,final,1);
	
    outFrag = vec4(position, 1.0f);//saturate(ao + base).xxxx;
}