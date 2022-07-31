#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 colour;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) in DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 colour;
} fs_in;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float ScreenPxRange()
{
	float pxRange = 2.0f;
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(textures[int(fs_in.tid)], 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(fs_in.uv);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

void main()
{
	vec4 bgColour = vec4(fs_in.colour.rgb, 0.0);
	vec4 fgColour = fs_in.colour;
	
	vec3 msd = texture(textures[int(fs_in.tid)], fs_in.uv).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = ScreenPxRange() * (sd - 0.5f);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    colour = mix(bgColor, fgColour, opacity);
}