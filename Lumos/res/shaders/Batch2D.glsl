#shader vertex

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in float tid;
layout (location = 3) in float mid;
layout (location = 4) in uint color;

layout(std140) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

out DATA
{
	vec4 position;
	vec2 uv;
	float tid;
	float mid;
	flat uint color;
} vs_out;

void main()
{
	gl_Position = ubo.projView * position;
	vs_out.position = position;
	vs_out.uv = uv;
	vs_out.tid = tid;
	vs_out.mid = mid;
	vs_out.color = color;
};

#shader fragment

layout (location = 0) out vec4 color;

in DATA
{
	vec4 position;
	vec2 uv;
	float tid;
	float mid;
	flat uint color;
} fs_in;

uniform sampler2D textures[32];

void main()
{
	vec4 texColor = vec4(0.4,0.3,0.0,1.0);
	if (fs_in.tid > 0.0)
	{
		int tid = int(fs_in.tid - 0.5);
		//texColor = texture(textures[tid], fs_in.uv);
	}

	color = texColor;
};
