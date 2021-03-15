#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 colour;

layout (location = 0) in DATA
{
	vec3 position;
	vec4 colour;
	vec2 size;
	vec2 uv;
} fs_in;

void main()
{

	float distSq = dot(fs_in.uv, fs_in.uv);
	if (distSq > 1.0)
	{
		discard;
	}
	colour = fs_in.colour;
}
