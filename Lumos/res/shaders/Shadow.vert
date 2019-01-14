#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

layout(set = 0,binding = 1) uniform UniformBufferObject2
{
    mat4 model;
} ubo2;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout (location = 0) in vec3 position;

void main()
{
    gl_Position = ubo.projView * ubo2.model * vec4(position, 1.0);
}