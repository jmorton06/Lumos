#shader vertex

layout (std140) uniform UniformBufferObject
{
    mat4 projView[16];
} ubo;

uniform int PushConstant;
uniform mat4 modelMatrix;

layout (location = 0) in vec3 position;

void main(void)
{
	gl_Position = ubo.projView[PushConstant] * modelMatrix * vec4(position, 1.0);
}

#shader fragment

layout(location = 0) out float depth;

void main(void)
{
	depth = gl_FragCoord.z;
}
