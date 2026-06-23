#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Octahedral.glslh"

layout(location = 0) out vec4 fragColour;
layout (location = 0) in vec2 outTexCoord;

layout (set = 0, binding = 0) uniform UniformBuffer
{
	mat4 view;
    vec2 ssaoTexelOffset;
	int ssaoBlurRadius;
	int pad;
	float near;
	float far;
} ubo;

layout (set = 0,binding = 1) uniform sampler2D in_SSAO;
layout (set = 0,binding = 2) uniform sampler2D in_Normal;
layout (set = 0,binding = 3) uniform sampler2D in_Depth;

// Linearise a Vulkan zero-to-one clip depth into a positive view-space distance.
float LinearizeDepth(float d)
{
	return (ubo.near * ubo.far) / (ubo.far - d * (ubo.far - ubo.near));
}

void main()
{
	float ourDepth = LinearizeDepth(texture(in_Depth, outTexCoord).r);
	vec3 ourNormal = normalize(mat3(ubo.view) * OctDecodeNormal(texture(in_Normal, outTexCoord).rg));

	int sampleCount = 0;
	float sum = 0.0f;
	for (int i = -ubo.ssaoBlurRadius; i <= ubo.ssaoBlurRadius; i++)
	{
		vec2 offset = ubo.ssaoTexelOffset * float(i);
		float depth = LinearizeDepth(texture(in_Depth, outTexCoord + offset).r);
		vec3 normal = normalize(mat3(ubo.view) * OctDecodeNormal(texture(in_Normal, outTexCoord + offset).rg));
		// Raw [0,1] depth is non-linear, so a fixed epsilon rejected every
		// neighbour on sloped/curved surfaces and the blur did nothing.
		// Compare linear depth with a distance-relative threshold instead.
		if (abs(ourDepth - depth) < ourDepth * 0.05f && dot(ourNormal, normal) > 0.85f)
		{
			sum += texture(in_SSAO, outTexCoord + offset).r;
			++sampleCount;
		}
	}
	fragColour = clamp(sum / float(sampleCount), 0.0f, 1.0f).xxxx;
}
