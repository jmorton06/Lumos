#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec4 outPosition;

layout(set = 0, binding = 1) uniform samplerCube u_CubeMap;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(u_CubeMap, outPosition.xyz);
}
