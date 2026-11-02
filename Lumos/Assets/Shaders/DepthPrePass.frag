#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"
#include "PBR.glslh"
#include "Octahedral.glslh"
#define GBUFFER_MATERIAL
#include "GBuffer.glslh"

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) in VertexData VertexOutput;
// Packed G-buffer, see GBuffer.glslh
layout(location = 0) out vec4 OutGBuffer;

void main(void)
{
	vec2 m    = GBufferMaterial(VertexOutput.TexCoord);
	vec2 octN = OctEncodeNormal(normalize(VertexOutput.Normal));
	// depth from the interpolated position, not the rasteriser's: on some GPUs a huge triangle clipped
	// by the camera gets its depth set up with large error (a step along the diagonal of a big floor)
	vec4 clip  = u_CameraData.projView * vec4(VertexOutput.Position.xyz, 1.0);
	OutGBuffer = vec4(octN, clip.z / clip.w, GBufferPackMaterial(m.x, m.y));
}
