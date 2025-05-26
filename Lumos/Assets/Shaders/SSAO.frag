#version 450
#include "Common.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout (set = 0, binding = 1) uniform sampler2D in_Normal;
layout (set = 0, binding = 2) uniform sampler2D in_Noise;
layout (set = 0, binding = 3) uniform sampler2D in_Depth;

layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 0) uniform UniformBuffer
{
	mat4 projection;
	mat4 invProj;
	mat4 view;
	// SSAO Gen Data
	vec4 samples[64];
	float ssaoRadius;
    float near;
    float far;
    float strength;
} ubo;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * ubo.near * ubo.far) / (ubo.far + ubo.near - z * (ubo.far - ubo.near));
}

vec3 reconstructVSPosFromDepth(vec2 uv)
{
    float depth = texture(in_Depth, uv).r;
    float z = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewPos = ubo.invProj * clipPos;
    return viewPos.xyz / viewPos.w;
}

const int MAX_KERNEL_SIZE = 32;
const float INV_MAX_KERNEL_SIZE_F = 1.0/float(MAX_KERNEL_SIZE);
const vec2 HALF_2 = vec2(0.5);

#define MAX_DISTANCE 1.0

vec3 GetNormal(vec2 uv)
{
    return normalize(texture(in_Normal, uv).xyz * 2.0f - 1.0f);
}

void main()
{
    float depth = texture(in_Depth, outTexCoord).r;

	if (depth < 0.0001f)
	{
		fragColour = vec4(1.0);
		return;
	}

	vec3 normal = GetNormal(outTexCoord.xy);
	vec3 posVS = reconstructVSPosFromDepth(outTexCoord);

	ivec2 depthTexSize = textureSize(in_Depth, 0);
	ivec2 noiseTexSize = textureSize(in_Noise, 0);
	float renderScale = 0.5; // SSAO is rendered at 0.5x scale
	vec2 noiseUV = vec2(float(depthTexSize.x)/float(noiseTexSize.x), float(depthTexSize.y)/float(noiseTexSize.y)) * outTexCoord * renderScale;

	vec3 randomVec = texture(in_Noise, noiseUV).xyz;

	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	//float bias = 0.001f;
	float bias = 0.001f;

	float occlusion = 0.0f;
	int sampleCount = 0;
	for (uint i = 0; i < MAX_KERNEL_SIZE; i++)
	{
		vec3 samplePos = TBN * ubo.samples[i].xyz;
		samplePos = posVS + samplePos * ubo.ssaoRadius;

		vec4 offset = vec4(samplePos, 1.0f);
		offset = ubo.projection * offset;
		offset.xy /= offset.w;
		offset.xy = (offset.xy * 0.5f) + HALF_2;
		//offset.y = 1.0 - offset.y;

		vec3 sampledNormal = GetNormal(offset.xy);
		vec3 reconstructedPos = reconstructVSPosFromDepth(offset.xy);

		if (dot(sampledNormal, normal) > 0.7)
        {
            ++sampleCount;
        }
        else
        {
            float rangeCheck = smoothstep(0.0f, 1.0f, ubo.ssaoRadius / abs(-reconstructedPos.z + samplePos.z - bias));
            occlusion += (reconstructedPos.z > samplePos.z - bias ? 1.0f : 0.0f) * rangeCheck;
            ++sampleCount;
        }

	}
	//occlusion = 1.0f - (occlusion * INV_MAX_KERNEL_SIZE_F);
	//occlusion = pow(occlusion, ubo.strength);

	occlusion = clamp(1.0 - occlusion * INV_MAX_KERNEL_SIZE_F, 0.0, 1.0);
	occlusion = pow(occlusion, ubo.strength);

	fragColour = occlusion.xxxx;
}
