#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform PushConsts
{
	uint cascadeIndex;
} pushConsts;

layout(set = 0,binding = 0) uniform UniformBufferObject
{
    mat4 projView[16];
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
    gl_Position = vec4(position, 1.0) * ubo2.model * ubo.projView[pushConsts.cascadeIndex];
}