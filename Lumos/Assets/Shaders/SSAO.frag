#version 450
#include "Common.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 ex_TexCoord;

layout (set = 0, binding = 1) uniform sampler2D in_Depth;
layout (set = 0, binding = 2) uniform sampler2D in_Normal;
layout (set = 0, binding = 3) uniform sampler2D in_Noise;

layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 0) uniform UniformBuffer
{
	mat4 projection;
	mat4 invProj;
	// SSAO Gen Data
	vec4 samples[64];
	float ssaoRadius;
    float p0;
    float p1;
    float p2;
} ubo;

vec3 reconstructVSPosFromDepth(vec2 uv)
{
	float depth = texture(in_Depth, uv).r;
	depth = depth * 2.0 - 1.0; // Remap depth from [0, 1] to [-1, 1]
	float x = uv.x * 2.0 - 1.0;
	float y = (1.0 - uv.y) * 2.0 - 1.0;
  vec4 pos = vec4(x, y, depth, 1.0);
  vec4 posVS = ubo.invProj * pos;
  return posVS.xyz / posVS.w;
}

void main()
{ 
    float depth = texture(in_Depth, ex_TexCoord).r;
	
	if (depth == 0.0f)
	{
		fragColour = vec4(1.0);
		return;
	}

	vec3 normal = normalize(texture(in_Normal, ex_TexCoord).rgb * 2.0f - 1.0f);

	vec3 posVS = reconstructVSPosFromDepth(ex_TexCoord);

	ivec2 depthTexSize = textureSize(in_Depth, 0); 
	ivec2 noiseTexSize = textureSize(in_Noise, 0);
	float renderScale = 0.5; // SSAO is rendered at 0.5x scale
	vec2 noiseUV = vec2(float(depthTexSize.x)/float(noiseTexSize.x), float(depthTexSize.y)/float(noiseTexSize.y)) * ex_TexCoord * renderScale;
	// noiseUV += vec2(0.5);
	vec3 randomVec = texture(in_Noise, noiseUV).xyz;
	
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float bias = 0.01f;

	float occlusion = 0.0f;
	int sampleCount = 0;
	for (uint i = 0; i < 64; i++)
	{
		vec3 samplePos = TBN * ubo.samples[i].xyz;
		samplePos = posVS + samplePos * ubo.ssaoRadius; 

		vec4 offset = vec4(samplePos, 1.0f);
		offset = ubo.projection * offset;
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5f + 0.5f;
		offset.y = 1.0f - offset.y;
		
		vec3 reconstructedPos = reconstructVSPosFromDepth(offset.xy);
        //occlusion += (reconstructedPos.z <= samplePos.z - bias ? 1.0 : 0.0);

		vec3 sampledNormal = normalize(texture(in_Normal, offset.xy).xyz * 2.0f - 1.0f);
		if (dot(sampledNormal, normal) > 0.99)
		{
			++sampleCount;
		}
		else
		{
			float rangeCheck = smoothstep(0.0f, 1.0f, ubo.ssaoRadius / abs(reconstructedPos.z - samplePos.z - bias));
			occlusion += (reconstructedPos.z <= samplePos.z - bias ? 1.0f : 0.0f) * rangeCheck;
			++sampleCount;
		}
	}
	occlusion = 1.0 - (occlusion / float(max(sampleCount,1)));
	
	fragColour = occlusion.xxxx;
}