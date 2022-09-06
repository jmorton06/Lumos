#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec2 tid;
layout (location = 3) in vec4 colour;

layout(set = 0,binding = 0) uniform UBO
{
	mat4 projView;
} ubo;

layout (location = 0) out DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 colour;
} vs_out;

void main()
{
	gl_Position =  ubo.projView * vec4(position,1.0);
	vs_out.position = position;
	vs_out.uv = uv;
	vs_out.tid = tid.x;
	vs_out.colour = colour;
}