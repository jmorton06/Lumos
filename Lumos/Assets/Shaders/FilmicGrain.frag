#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;

float Gaussian(float z, float u, float o) 
{
    return (1.0 / (o * sqrt(2.0 * 3.1415))) * exp(-(((z - u) * (z - u)) / (2.0 * (o * o))));
}

layout(push_constant) uniform PushConsts
{
	 float time;
     float filmGrainIntensity;
} pushConsts;

void main()
{
	vec4 colour = texture(u_Texture, outTexCoord);
	vec2 uv = outTexCoord;
	// Film grain
    float t          = pushConsts.time * 1.0f;
    float seed       = dot(uv, vec2(12.9898, 78.233));
    float noise      = fract(sin(seed) * 43758.5453 + t);
    noise            = Gaussian(noise, 0.5, 0.25);
    float filmGrain =  noise * pushConsts.filmGrainIntensity;
	
    colour.rgb += vec3(filmGrain, filmGrain, filmGrain);	
	outFrag = vec4(clamp(colour.rgb, 0.0, 1.0), 1.0f);
}