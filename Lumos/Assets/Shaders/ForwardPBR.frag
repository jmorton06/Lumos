#version 450
#include "Buffers.glslh"
#include "PBR.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) in VertexData VertexOutput;

float ShadowFade = 1.0;
float InvSqrtPCFSamples = 1.0;

layout(location = 0) out vec4 outColour;

const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

struct Material
{
	vec4 Albedo;
	float Metallic;
	float Roughness;
	float PerceptualRoughness;
	float Reflectance;
	vec3 Emissive;
	vec3 Normal;
	float AO;
	vec3 View;
	float NDotV;
	vec3 F0;
	vec3 EnergyCompensation;
	vec2 dfg;
};

vec4 GetAlbedo()
{
	if(u_MaterialData.AlbedoMapFactor < 0.05)
		return  u_MaterialData.AlbedoColour;

	return u_MaterialData.AlbedoColour * (u_MaterialData.AlbedoMapFactor * DeGamma(texture(u_AlbedoMap, VertexOutput.TexCoord)));
}

vec3 GetMetallic()
{
	if(u_MaterialData.MetallicMapFactor < 0.05)
		return  u_MaterialData.Metallic.rrr;

	return (1.0 - u_MaterialData.MetallicMapFactor) * u_MaterialData.Metallic + u_MaterialData.MetallicMapFactor * texture(u_MetallicMap, VertexOutput.TexCoord).rgb;
}

float GetRoughness()
{
	if(u_MaterialData.RoughnessMapFactor < 0.05)
		return  u_MaterialData.Roughness;
	return (1.0 - u_MaterialData.RoughnessMapFactor) * u_MaterialData.Roughness + u_MaterialData.RoughnessMapFactor * texture(u_RoughnessMap, VertexOutput.TexCoord).r;
}

float GetAO()
{
	if(u_MaterialData.AOMapFactor < 0.05)
		return 1.0;

	return (1.0 - u_MaterialData.AOMapFactor) + u_MaterialData.AOMapFactor * texture(u_AOMap, VertexOutput.TexCoord).r;
}

vec3 GetEmissive(vec3 albedo)
{
	if(u_MaterialData.EmissiveMapFactor < 0.05)
		return (u_MaterialData.Emissive * albedo);
	return (u_MaterialData.Emissive * albedo) + u_MaterialData.EmissiveMapFactor * DeGamma(texture(u_EmissiveMap, VertexOutput.TexCoord).rgb);
}

const vec2 PoissonDistribution16[16] = vec2[](
											  vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725), vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
											  vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464), vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
											  vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420), vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
											  vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590), vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
											  );


vec2 SamplePoisson(int index)
{
	return PoissonDistribution16[index % 16];
}

vec2 VogelDiskSample(int sampleIndex, float invSquareRootSamplesCount, float phi)
{
	float GoldenAngle = 2.4;

	float r = sqrt(sampleIndex + 0.5) * invSquareRootSamplesCount;/// sqrt(samplesCount);
	float theta = sampleIndex * GoldenAngle + phi;

    float sine = sin(theta);
    float cosine = cos(theta);

    return vec2(r * cosine, r * sine);
}

float Random(vec4 seed4)
{
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float Random(vec3 seed, int i)
{
	vec4 seed4 = vec4(seed,i);
	return Random(seed4);
}

float GetShadowBias(vec3 lightDirection, vec3 normal, int shadowIndex)
{
	float minBias = u_SceneData.InitialBias;
	float bias = max(minBias * (1.0 - dot(normal, lightDirection)), minBias);
	return bias;
}

float InterleavedGradientNoise(vec2 screenPosition)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPosition, magic.xy)));
}

float PCFShadowDirectionalLight(sampler2DArray shadowMap, vec4 shadowCoords, float uvRadius, vec3 lightDirection, vec3 normal, vec3 wsPos, int cascadeIndex)
{
	float bias = GetShadowBias(lightDirection, normal, cascadeIndex);
	float sum = 0.0;
	float invSquareRootSamplesCount = InvSqrtPCFSamples;

	#define MAX_PCF_SAMPLES 16
	int sampleCount = min(u_SceneData.PCFSamples, MAX_PCF_SAMPLES);
	for (int i = 0; i < sampleCount; i++)
	{

		vec2 offset;

		if(u_SceneData.VogelOffset > 0)
		{
	    	float noise = InterleavedGradientNoise(gl_FragCoord.xy) * TwoPI;
			offset = VogelDiskSample(i, invSquareRootSamplesCount, noise) * uvRadius;
		}
		else
		{
		    int index = int(16.0f*Random(vec4(wsPos, i)))%16;
			offset = SamplePoisson(index) * uvRadius;
		}

		float z = texture(shadowMap, vec3(shadowCoords.xy + offset, cascadeIndex)).r - bias;
		sum += step(shadowCoords.z, z);
	}

	return sum / float(sampleCount);
}

int CalculateCascadeIndex(vec3 wsPos)
{
	vec4 viewPos = u_SceneData.ViewMatrix * vec4(wsPos, 1.0);
    float z = viewPos.z;
    vec4 comparison = vec4(
        step(z, u_SceneData.SplitDepths[0]),
        step(z, u_SceneData.SplitDepths[1]),
        step(z, u_SceneData.SplitDepths[2]),
        step(z, u_SceneData.SplitDepths[3])
    );
    int cascadeIndex = int(dot(comparison, vec4(1.0)));
	return min(cascadeIndex, u_SceneData.ShadowCount - 1);
}

float CalculateShadow(vec3 wsPos, int cascadeIndex, vec3 lightDirection, vec3 normal)
{
	float shadowDistance     = u_SceneData.MaxShadowDist;
	float transitionDistance = u_SceneData.ShadowFade;

	vec4 viewPos = u_SceneData.ViewMatrix * vec4(wsPos, 1.0);
	float distance = length(viewPos);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0);

	vec4 shadowCoord = u_SceneData.BiasMatrix * u_SceneData.ShadowTransform[cascadeIndex] * vec4(wsPos, 1.0);
	shadowCoord = shadowCoord * (1.0 / shadowCoord.w);

	if(shadowCoord.z > 0.999 || shadowCoord.z < 0.0)
		return 1.0;

	float shadowAmount = 1.0;

	float uvRadius = 0.002;

	if (u_SceneData.FilterShadows  == 1)
	{
		float NEAR = 0.01;
		uvRadius =  u_SceneData.LightSize * NEAR / shadowCoord.z;
		uvRadius = min(uvRadius, 0.005);

		shadowAmount = PCFShadowDirectionalLight(uShadowMap, shadowCoord, uvRadius, lightDirection, normal, wsPos, cascadeIndex);
	}
	else
	{
		float bias = GetShadowBias(lightDirection, normal, cascadeIndex);
		float z = texture(uShadowMap, vec3(shadowCoord.xy, cascadeIndex)).r;
		shadowAmount = step(shadowCoord.z - bias, z);
	}

	if (u_SceneData.BlendShadows == 1)
	{
		float splitDist = u_SceneData.SplitDepths[cascadeIndex];
		float fadeDist = u_SceneData.CascadeFade * 0.5;
		float cascadeFade = smoothstep(splitDist + fadeDist, splitDist - fadeDist, viewPos.z);
		int cascadeNext = cascadeIndex + 1;
		if (cascadeFade > 0.0 && cascadeNext < u_SceneData.ShadowCount)
		{
			vec4 shadowCoordNext = u_SceneData.BiasMatrix * u_SceneData.ShadowTransform[cascadeNext] * vec4(wsPos, 1.0);
			shadowCoordNext = shadowCoordNext * (1.0 / shadowCoordNext.w);
			float shadowAmount1 = PCFShadowDirectionalLight(uShadowMap, shadowCoordNext, uvRadius, lightDirection, normal, wsPos, cascadeNext);

			shadowAmount =  mix(shadowAmount, shadowAmount1, cascadeFade);
		}
	}

	return 1.0 - ((1.0 - shadowAmount) * ShadowFade);
}


vec3 IsotropicLobe(const Material material, const Light light, const vec3 h,
                   float NoV, float NoL, float NoH, float LoH) {

    float D = distribution(material.Roughness, NoH, material.Normal, h);
    float V = visibility(material.Roughness, NoV, NoL);
    vec3  F = fresnel(material.F0, LoH);

    return (D * V) * F;
}

vec3 DiffuseLobe(const Material material, float NoV, float NoL, float LoH)
{
    return material.Albedo.xyz * Diffuse(material.Roughness, NoV, NoL, LoH);
}

vec3 SpecularLobe(const Material material, const Light light, const vec3 h, float NoV, float NoL, float NoH, float LoH)
{
    return IsotropicLobe(material, light, h, NoV, NoL, NoH, LoH);
}

vec3 Lighting(vec3 F0, vec3 wsPos, Material material)
{
	vec3 result = vec3(0.0);

	for(int i = 0; i < u_SceneData.LightCount; i++)
	{
		Light light = u_SceneData.lights[i];
		float value = 0.0;

		if(light.type == 2.0)
		{
		    // Vector to light
			vec3 L = light.position.xyz - wsPos;
			// Distance from light to fragment position
			float dist = length(L);

			// Light to fragment
			L = normalize(L);

			// Attenuation
			//float atten = light.radius / (pow(dist, 2.0) + 1.0);
			float attenuation = clamp(1.0 - (dist * dist) / (light.radius * light.radius), 0.0, 1.0);

			value = attenuation;

			light.direction = vec4(L,1.0);
		}
		else if (light.type == 1.0)
		{
			vec3 L = light.position.xyz - wsPos;
			float dist          = length(L);
			L = normalize(L);
			float theta         = dot(L.xyz, light.direction.xyz);
			float cutoffAngleF   = 1.0 - light.angle;
			float epsilon       = cutoffAngleF - cutoffAngleF * 0.9;
			float attenuation 	= ((theta - cutoffAngleF) / epsilon); // attenuate when approaching the outer cone
			attenuation         *= light.radius / (dist * dist + 1.0);//saturate(1.0 - distF / light.range);
			//float intensity 	= attenuation * attenuation;

			// Erase light if there is no need to compute it
			//intensity *= step(theta, cutoffAngle);

			value = clamp(attenuation, 0.0, 1.0);
		}
		else
		{
			float nDotL = dot(material.Normal, light.direction.xyz);

			if(u_SceneData.ShadowEnabled > 0 && nDotL > 0.0f)
			{
				int cascadeIndex = CalculateCascadeIndex(wsPos);
				value = CalculateShadow(wsPos,cascadeIndex, light.direction.xyz, material.Normal);
			}
			else
				value = 1.0;
		}


		vec3 Lradiance = light.colour.xyz * light.intensity;
		vec3 Li = light.direction.xyz;
		float lightNoL = saturate(dot(material.Normal, Li));
		vec3 h = normalize(material.View + Li);

		float shading_NoV = clampNoV(dot(material.Normal, material.View));
    	float NoV = shading_NoV;
    	float NoL = saturate(lightNoL);
    	float NoH = saturate(dot(material.Normal, h));
    	float LoH = saturate(dot(Li, h));

    	vec3 Fd = DiffuseLobe(material, NoV, NoL, LoH);
		vec3 Fr = SpecularLobe(material, light, h, NoV, NoL, NoH, LoH);;

		vec3 colour = Fd + Fr;// * material.EnergyCompensation;

		result += (colour * Lradiance.rgb) * (value * NoL * ComputeMicroShadowing(NoL, material.AO));
	}
	return result;
}

vec3 IBL(vec3 F0, vec3 Lr, Material material)
{
	vec3 irradiance = texture(uIrrMap, material.Normal).rgb;

	vec3 E = F0 * material.dfg.x + material.dfg.y;
	vec3 kd = (1.0 - E) * (1.0 - material.Metallic.x);
	vec3 diffuseIBL = material.Albedo.xyz * irradiance;

	int u_EnvRadianceTexLevels = u_SceneData.EnvMipCount;
	vec3 specularIrradiance = textureLod(uEnvMap, Lr, material.PerceptualRoughness * u_EnvRadianceTexLevels).rgb;

	vec3 specularIBL = specularIrradiance * E;

	return kd * diffuseIBL + specularIBL;
}

void main()
{
	vec4 texColour = GetAlbedo();
	if(texColour.w < u_MaterialData.AlphaCutOff)
		discard;

	float metallic  = 0.0;
	float roughness = 0.0;

	if(u_MaterialData.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		metallic  = GetMetallic().x;
		roughness = GetRoughness();
	}
	else if( u_MaterialData.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex  = texture(u_MetallicMap, VertexOutput.TexCoord).rgb;
		metallic  = (1.0 - u_MaterialData.MetallicMapFactor) * u_MaterialData.Metallic + u_MaterialData.MetallicMapFactor * tex.b;
		roughness = (1.0 - u_MaterialData.MetallicMapFactor) * u_MaterialData.Roughness + u_MaterialData.MetallicMapFactor * tex.g;
	}
	else if( u_MaterialData.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		//TODO
		vec3 tex  = texture(u_MetallicMap, VertexOutput.TexCoord).rgb;
		metallic  = (1.0 - u_MaterialData.MetallicMapFactor) * u_MaterialData.Metallic + u_MaterialData.MetallicMapFactor * tex.b;
		roughness = (1.0 - u_MaterialData.MetallicMapFactor) * u_MaterialData.Roughness + u_MaterialData.MetallicMapFactor * tex.g;
	}

	Material material;
    material.Albedo    = texColour;
    material.Metallic  = metallic;
    material.PerceptualRoughness = roughness;
	material.Reflectance = u_MaterialData.Reflectance;
	material.Normal = normalize(VertexOutput.Normal);

	if (u_MaterialData.NormalMapFactor > 0.04)
	{
		material.Normal = normalize(texture(u_NormalMap, VertexOutput.TexCoord).rgb * 2.0f - 1.0f);
		material.Normal = normalize(VertexOutput.WorldNormal * material.Normal);
	}

	material.AO		   = GetAO();
	material.Emissive  = GetEmissive(material.Albedo.rgb);

	vec2 uv = gl_FragCoord.xy / vec2(u_SceneData.Width, u_SceneData.Height);
	float ssao = texture(uSSAOMap, uv).r;

    // Start from clamped perceptual
	material.PerceptualRoughness = clamp(material.PerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
	float roughness2 = material.PerceptualRoughness * material.PerceptualRoughness;

	// Specular anti-aliasing
#if !QUALITY_LOW
	{
		const float strength = 1.0;
		const float maxRoughnessGain = 0.02;

		vec3 dndu = dFdx(material.Normal);
		vec3 dndv = dFdy(material.Normal);
		float variance = dot(dndu, dndu) + dot(dndv, dndv);

		float kernelRoughness2 = min(variance * strength, maxRoughnessGain);

		float filteredRoughness2 = roughness2 + kernelRoughness2;
		filteredRoughness2 = clamp(filteredRoughness2, MIN_ROUGHNESS, 1.0);

		material.Roughness = sqrt(filteredRoughness2);
	}
#else
	material.Roughness = sqrt(roughness2);
#endif

	InvSqrtPCFSamples = 1.0 / sqrt(float(u_SceneData.PCFSamples));

	vec3 wsPos     = VertexOutput.Position.xyz;
	material.View  = normalize(u_SceneData.cameraPosition.xyz - wsPos);
	material.NDotV = max(dot(material.Normal, material.View), 1e-4);

	material.dfg = texture(uBRDFLUT, vec2(material.NDotV, material.PerceptualRoughness)).rg;
	float reflectance = computeDielectricF0(material.Reflectance);
	vec3 F0 = computeF0(material.Albedo, material.Metallic.x, reflectance);
	material.F0 = F0;
    material.EnergyCompensation = 1.0 + material.F0 * (1.0 / max(0.1, material.dfg.y) - 1.0);
	material.Albedo.xyz = computeDiffuseColour(material.Albedo, material.Metallic.x);
	material.Albedo *= ssao;

	vec3 Lr = 2.0 * material.NDotV * material.Normal - material.View;
	vec3 lightContribution = Lighting(material.F0, wsPos, material);
	vec3 iblContribution   = IBL(material.F0, Lr, material);

	vec3 finalColour = lightContribution + iblContribution + material.Emissive;
	outColour = vec4(finalColour, 1.0);

	if(u_SceneData.Mode > 0)
	{
		switch(u_SceneData.Mode)
		{
			case 1:
			outColour = material.Albedo;
			break;
			case 2:
			outColour = vec4(material.Metallic.rrr, 1.0);
			break;
			case 3:
			outColour = vec4(material.PerceptualRoughness.xxx,1.0);
			break;
			case 4:
			outColour = vec4(material.AO.xxx, 1.0);
			break;
			case 5:
			outColour = vec4(material.Emissive, 1.0);
			break;
			case 6:
			outColour = vec4(material.Normal,1.0);
			break;
            case 7:
			int cascadeIndex = CalculateCascadeIndex(wsPos);
			switch(cascadeIndex)
			{
				case 0 : outColour = outColour * vec4(0.8,0.2,0.2,1.0); break;
				case 1 : outColour = outColour * vec4(0.2,0.8,0.2,1.0); break;
				case 2 : outColour = outColour * vec4(0.2,0.2,0.8,1.0); break;
				case 3 : outColour = outColour * vec4(0.8,0.8,0.2,1.0); break;
			}
			break;
		}
	}
}
