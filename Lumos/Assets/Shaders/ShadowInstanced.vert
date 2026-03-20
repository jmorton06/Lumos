#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "BuffersInstanced.glslh"

layout(push_constant) uniform PushConsts
{
	uint cascadeIndex;
} pushConsts;

layout(std430, set = 3, binding = 0) readonly buffer InstanceTransforms
{
	mat4 transforms[];
} u_InstanceTransforms;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec2 uv;

void main()
{
    mat4 worldTransform = u_InstanceTransforms.transforms[gl_InstanceIndex];
    mat4 shadowProjView = u_DirShadow.DirLightMatrices[pushConsts.cascadeIndex];
    gl_Position = shadowProjView * worldTransform * vec4(inPosition, 1.0);

    vec4 test2 = inColor;
    uv = inTexCoord;
	vec3 test5 = inNormal;
	vec3 test3 = inTangent;
	vec3 test4 = inBitangent;
}
