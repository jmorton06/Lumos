#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "EnvironmentMapping.glslh"

// One face of an environment probe: each texel's direction, projected into the frame
// the probe camera just rendered (its HDR colour, before tone mapping).

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform sampler2D u_Texture;
layout(set = 0, binding = 1) uniform UniformBuffer
{
	mat4 ProjView;
	vec4 Position; // w = gain
} ubo;

layout(push_constant) uniform PushConsts
{
	float Roughness;
	uint cubeFaceIndex;
	float p0;
	float p1;
} pushConsts;

layout(location = 0) out vec4 FragColour;

void main()
{
	vec3 dir  = GetCubeMapTexCoord2(int(pushConsts.cubeFaceIndex), outTexCoord);
	vec4 clip = ubo.ProjView * vec4(ubo.Position.xyz + dir, 1.0);
	if(clip.w <= 0.0)
	{
		FragColour = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
	vec2 uv = clamp((clip.xy / clip.w) * 0.5 + 0.5, vec2(0.0), vec2(1.0));
	// clamp hot emitters: one bright pixel would ring through the whole prefilter
	FragColour = vec4(min(texture(u_Texture, uv).rgb * ubo.Position.w, vec3(32.0)), 1.0);
}
