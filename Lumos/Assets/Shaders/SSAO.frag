#version 450
#include "Common.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout (set = 0, binding = 1) uniform sampler2D in_Depth;
layout (set = 0, binding = 2) uniform sampler2D in_Normal;
layout (set = 0, binding = 3) uniform sampler2D in_Noise;

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
    return z;//(2.0 * ubo.near * ubo.far) / (ubo.far + ubo.near - z * (ubo.far - ubo.near));	
}

    // TODO:  Crytek's view frustum ray method 
vec3 reconstructVSPosFromDepth(vec2 uv)
{
	float depth = texture(in_Depth, uv).r;
	depth = LinearizeDepth(depth); // Remap depth from [0, 1] to [-1, 1]
	float x = uv.x * 2.0 - 1.0;
	float y = (uv.y) * 2.0 - 1.0;
  	vec4 pos = vec4(x, y, depth, 1.0);
  	vec4 posVS = ubo.invProj * pos;
  	return posVS.xyz / posVS.w;
}

const int MAX_KERNEL_SIZE = 32;
const float INV_MAX_KERNEL_SIZE_F = 1.0/float(MAX_KERNEL_SIZE);
const vec2 HALF_2 = vec2(0.5);

#define MAX_DISTANCE 1.0

void main()
{ 
    float depth = texture(in_Depth, outTexCoord).r;
	
	if (depth < 0.0001f)
	{
		fragColour = vec4(1.0);
		return;
	}

	vec3 normal = normalize(mat3(ubo.view) * (texture(in_Normal, outTexCoord).xyz * 2.0f - 1.0f));
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
	float bias = 0.01f;
	
	float occlusion = 0.0f;
	int sampleCount = 0;
	for (uint i = 0; i < MAX_KERNEL_SIZE; i++)
	{
		vec3 samplePos = TBN * ubo.samples[i].xyz;
		samplePos = posVS + samplePos * ubo.ssaoRadius; 

		vec4 offset = vec4(samplePos, 1.0f);
		offset = ubo.projection * offset;
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5f + HALF_2;

		vec3 sampledNormal = normalize(mat3(ubo.view) * (texture(in_Normal, offset.xy).xyz * 2.0f - 1.0f));
		 vec3 reconstructedPos = reconstructVSPosFromDepth(offset.xy);

		if (dot(sampledNormal, normal) > 0.99)
		{
			++sampleCount;
		}
		else
		{
			//float rangeCheck = smoothstep(0.0f, 1.0f, ubo.ssaoRadius / abs(reconstructedPos.z - samplePos.z - bias));
			//occlusion += (reconstructedPos.z <= samplePos.z - bias ? 1.0f : 0.0f) * rangeCheck;
			
			vec3 diff = posVS - reconstructedPos;
			float l = length(diff);
			
			float rangeCheck = smoothstep(0.0f, 1.0f, ubo.ssaoRadius / abs(reconstructedPos.z - samplePos.z - bias));//abs(diff.z - bias));
            occlusion += (reconstructedPos.z >= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck; 
			//occlusion *= smoothstep(MAX_DISTANCE,MAX_DISTANCE * 0.5, l);
			
			++sampleCount;
		}
	}
	occlusion = 1.0f - (occlusion * INV_MAX_KERNEL_SIZE_F) / ubo.strength;// / float(max(sampleCount,1)));
	//occlusion = pow(occlusion, 2.0f);
	//fragColour = vec4(posVS,1.0);
	fragColour = occlusion.xxxx;
}