#shader vertex

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 size;

layout(std140) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

out DATA
{
	vec3 position;
	vec4 color;
	vec2 size;
} vs_out;

void main()
{
	gl_Position = vec4(position,1.0) * ubo.projView;
	vs_out.position = position;
	vs_out.color = color;
	vs_out.size = size;
}

#shader end

#shader fragment

layout (location = 0) out vec4 color;

in DATA
{
	vec3 position;
	vec4 color;
	vec2 size;
} fs_in;

void main()
{
	color = fs_in.color;
}
#shader end