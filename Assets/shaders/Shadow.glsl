#shader vertex

layout (std140) uniform UniformBufferObject
{
    mat4 projView[16];
} ubo;

uniform mat4 modelMatrix;
uniform int PushConstant;

layout (location = 0) in vec3 position;

void main(void)
{
	gl_Position = vec4(position, 1.0) * modelMatrix * ubo.projView[PushConstant];
}

#shader fragment

layout(location = 0) out float depth;

void main(void)
{
	depth = gl_FragCoord.z;
}
