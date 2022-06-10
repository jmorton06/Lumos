#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;

layout(set = 0, binding = 0) uniform UniformBuffer
{
	float chromaticAberrationIntensity;
	float cameraAperture;
	 float p0;
	float p1;
} ubo;


void main()
{
	float camera_error = 1.0f / ubo.cameraAperture;
    float intensity    = camera_error * ubo.chromaticAberrationIntensity;
    vec2 shift       = vec2(intensity, -intensity);
	vec2 uv = outTexCoord;
	
    // Lens effect
    shift.x *= abs(uv.x * 2.0f - 1.0f);
    shift.y *= abs(uv.y * 2.0f - 1.0f);
    
    // Sample color
    vec3 colour = vec3(0.0f,0.0f,0.0f);
	vec2 g_texel_size = 1.0f / textureSize(u_Texture, 0);
    colour.r      = texture(u_Texture, uv + (g_texel_size * shift)).r;
    colour.g      = texture(u_Texture, uv).g;
    colour.b      = texture(u_Texture, uv - (g_texel_size * shift)).b;
	
	//vec3 colour = texture(u_Texture, outTexCoord).;
	outFrag = vec4(colour, 1.0);
}