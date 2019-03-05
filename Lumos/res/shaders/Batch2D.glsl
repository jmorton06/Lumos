#shader vertex

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in float tid;
layout (location = 3) in vec4 color;

layout(std140) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

out DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 color;
} vs_out;

void main()
{
	gl_Position = ubo.projView * vec4(position,1.0);
	vs_out.position = position;
	vs_out.uv = uv;
	vs_out.tid = tid;
	vs_out.color = color;
}

#shader end

#shader fragment

layout (location = 0) out vec4 color;

in DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 color;
} fs_in;

uniform sampler2D textures[16];

void main()
{
	vec4 texColor = fs_in.color;
	if (fs_in.tid > 0.0)
	{
		int tid = int(fs_in.tid - 0.5);
		texColor = texture(textures[tid], fs_in.uv);
	}

	color = texColor;
}
#shader end