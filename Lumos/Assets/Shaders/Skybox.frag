#version 450
#include "Common.glslh"
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec4 outPosition;

layout(set = 0, binding = 1) uniform samplerCube u_CubeMap;
layout(set = 0, binding = 2) uniform UniformBuffer
{
	float Exposure;
	int Mode; //0 env map , 1 custom sky
	float BlurLevel;
	float p1;
} data;

layout(location = 0) out vec4 outFrag;

const float coeiff = 0.3;
const vec3 totalSkyLight = vec3(0.3, 0.5, 1.0);

const float blurLevel = 1.0f;
const float timeCounter = 0.0f;


vec3 mie(float dist, vec3 sunL)
{
	return max(exp(-pow(dist, 0.25)) * sunL - 0.4, 0.0);
}

vec3 GetSky()
{
	vec3 uv = normalize(outPosition.xyz);

	vec3 sunPos;
	float radian = PI * 2.0 *((timeCounter / 86400.0) - 0.333333);
	sunPos.x = cos(radian);
	sunPos.y = sin(radian);
	sunPos.z = 0.0;

	float sunDistance = length(uv - normalize(sunPos));

	float scatterMult = clamp(sunDistance, 0.0, 1.0);
	float dist = uv.y + 0.1; // reduce horizon level
	dist = (coeiff * mix(scatterMult, 1.0, dist)) / dist;

	vec3 colour = dist * totalSkyLight;

	colour = max(colour, 0.0);
	colour = max(mix(pow(colour, 1.0 - colour),
		colour / (2.0 * colour + 0.5 - colour),
									  clamp(sunPos.y * 2.0, 0.0, 1.0)), 0.0);
	
	return colour;
}

void main()
{
	vec3 colour = vec3(1.0, 0.0,0.0);

	if(data.Mode == 0)
	{
		colour = DeGamma(textureLod(u_CubeMap, outPosition.xyz, data.BlurLevel).xyz);
	}
	else
	{
		colour = GetSky();
	}

	colour *= data.Exposure; 
	outFrag = vec4(colour, 1.0);
}


