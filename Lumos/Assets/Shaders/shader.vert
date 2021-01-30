#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UniformBufferObject 
{    
	mat4 proj;
    mat4 view;

} ubo;

layout(set = 0,binding = 1) uniform UniformBufferObject2 
{
    mat4 model;
} ubo2;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
    gl_Position =vec4(inPosition, 1.0) * ubo2.model * ubo.view * ubo.proj;
    fragColor = inColor;
	fragTexCoord = inTexCoord;
}