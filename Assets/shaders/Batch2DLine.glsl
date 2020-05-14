#shader vertex

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

layout(std140) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

out DATA
{
	vec3 position;
	vec4 color;
} vs_out;

void main()
{
	gl_Position = vec4(position,1.0) * ubo.projView;
	vs_out.position = position;
	vs_out.color = color;
}

#shader end

#shader fragment

layout (location = 0) out vec4 color;

in DATA
{
	vec3 position;
	vec4 color;
} fs_in;

void main()
{
	color = fs_in.color;
}
#shader end