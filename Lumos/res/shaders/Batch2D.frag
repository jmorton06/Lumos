#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 color;

layout (location = 0) in DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 color;
} fs_in;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

void main()
{
	vec4 texColor = fs_in.color;
	if (fs_in.tid > 0.0)
	{
		int tid = int(fs_in.tid - 0.5);
		texColor = texColor * texture(textures[tid], fs_in.uv);
	}

	color = texColor;
}
