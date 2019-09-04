#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 color;

layout(std140, binding = 0) uniform UniformBuffer
{
	float u_Scale;
	float u_Res;
} ubo;

layout(location = 0) in vec2 v_TexCoord;

float grid(vec2 st, float res)
{
	vec2 grid = fract(st);
	return step(res, grid.x) * step(res, grid.y);
}
 
void main()
{
	float scale = ubo.u_Scale;
	float resolution = ubo.u_Res;

	float x = grid(v_TexCoord * scale, resolution);
	color = vec4(vec3(0.2), 0.5) * (1.0 - x);
}