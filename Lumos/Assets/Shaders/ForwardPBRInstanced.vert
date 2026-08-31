#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "BuffersInstanced.glslh"

layout(std430, set = 3, binding = 0) readonly buffer InstanceBuffer
{
	vec4 data[];
} u_Instances;

const int INSTANCE_VEC4S = 5;

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
	int base = gl_InstanceIndex * INSTANCE_VEC4S;
	mat4 transform = mat4(u_Instances.data[base + 0], u_Instances.data[base + 1],
	                      u_Instances.data[base + 2], u_Instances.data[base + 3]);
	VertexOutput.Position = transform * vec4(inPosition, 1.0);
    gl_Position = u_CameraData.projView * VertexOutput.Position;

	// Per-instance albedo (5th vec4).
	VertexOutput.Colour = u_Instances.data[base + 4].xyz;
	VertexOutput.TexCoord = inTexCoord;

	mat3 transposeInv = transpose(inverse(mat3(transform)));

	vec3 N = normalize(transposeInv * inNormal);
	vec3 T = transposeInv * inTangent;
	T = normalize(T - dot(T, N) * N);
	vec3 Braw = transposeInv * inBitangent;
	vec3 Bcross = cross(N, T);
	float bs = sign(dot(Bcross, Braw));
	vec3 B = Bcross * (bs != 0.0 ? bs : 1.0);

	VertexOutput.Normal = N;
	VertexOutput.WorldNormal = mat3(T, B, N);
}
