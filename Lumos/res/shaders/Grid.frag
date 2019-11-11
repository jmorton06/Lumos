#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 color;

layout(set = 0,binding = 1) uniform UniformBuffer
{
	vec4 u_CameraPos;
	float u_Scale;
	float u_Res;
	float u_MaxDistance;
	float p1;
} ubo;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_Position;

float grid(vec2 st, float res)
{
	vec2 grid = fract(st);
	return step(res, grid.x) * step(res, grid.y);
}

void main()
{
	float scale = ubo.u_Scale;
	float resolution = ubo.u_Res;

	float alpha = 0.5 * (ubo.u_MaxDistance - length(ubo.u_CameraPos.xyz - v_Position)) / ubo.u_MaxDistance;
	
	alpha = max(0.0f, alpha);
	float x = grid(v_TexCoord * scale, resolution);
	color = vec4(vec3(0.2), alpha) * (1.0 - x);

	if(color.w < 0.3)
		discard;
}