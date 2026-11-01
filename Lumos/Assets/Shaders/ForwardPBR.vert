#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

// Graphics::PackedVertex (suffixes give the formats, see VKShader)
layout(location = 0) in vec3 inPosition_f32x3;
layout(location = 1) in vec4 inNormal_snorm16;
layout(location = 2) in vec4 inTangent_snorm16; // w = bitangent sign
layout(location = 3) in vec2 inTexCoord_f32x2;
layout(location = 4) in vec4 inColor_unorm8;

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
	VertexOutput.Position = pushConsts.transform * vec4(inPosition_f32x3, 1.0);
    gl_Position = u_CameraData.projView * VertexOutput.Position;

	VertexOutput.Colour = inColor_unorm8.xyz;
	VertexOutput.TexCoord = inTexCoord_f32x2;

	// cofactor matrix = inverse-transpose * det: no per-vertex inverse, and the scale goes in normalize
	mat3 m3 = mat3(pushConsts.transform);
	mat3 transposeInv = mat3(cross(m3[1], m3[2]), cross(m3[2], m3[0]), cross(m3[0], m3[1]));
	float detSign = sign(dot(m3[0], transposeInv[0]));
	transposeInv *= detSign;

	vec3 N = normalize(transposeInv * inNormal_snorm16.xyz);
	vec3 T = transposeInv * inTangent_snorm16.xyz;
	T = normalize(T - dot(T, N) * N);
	// a mirroring transform flips the handedness too
	vec3 B = cross(N, T) * (inTangent_snorm16.w < 0.0 ? -1.0 : 1.0) * detSign;

	VertexOutput.Normal = N;
	VertexOutput.WorldNormal = mat3(T, B, N);

}
