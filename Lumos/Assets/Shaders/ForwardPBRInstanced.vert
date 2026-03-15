#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "BuffersInstanced.glslh"

layout(std430, set = 3, binding = 0) readonly buffer InstanceTransforms
{
	mat4 transforms[];
} u_InstanceTransforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) out VertexData VertexOutput;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
	mat4 transform = u_InstanceTransforms.transforms[gl_InstanceIndex];
	VertexOutput.Position = transform * vec4(inPosition, 1.0);
    gl_Position = u_CameraData.projView * VertexOutput.Position;

	VertexOutput.Colour = inColor.xyz;
	VertexOutput.TexCoord = inTexCoord;

	mat3 transposeInv = transpose(inverse(mat3(transform)));

	vec3 N = normalize(transposeInv * inNormal);
	vec3 T = normalize(transposeInv * inTangent);
	vec3 B = normalize(cross(N, T));

	VertexOutput.Normal = N;
	VertexOutput.WorldNormal = mat3(T, B, N);
}
