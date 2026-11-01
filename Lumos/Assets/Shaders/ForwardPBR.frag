#version 450
#include "Buffers.glslh"
#include "PBR.glslh"
#include "GI.glslh"

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
float ContactShadow = 1.0;
// the surface's own normal: shadow offsets follow it, not the normal map (per-pixel bumps made speckled acne)
vec3 GeometricNormal = vec3(0.0, 1.0, 0.0);
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
#ifdef INSTANCED
	vec4 baseColour = vec4(VertexOutput.Colour, 1.0);
#else
	vec4 baseColour = u_MaterialData.AlbedoColour;
#endif

	if(u_MaterialData.AlbedoMapFactor < 0.05)
		return baseColour;

	return baseColour * (u_MaterialData.AlbedoMapFactor * DeGamma(texture(u_AlbedoMap, VertexOutput.TexCoord)));
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
	vec3 emissive = u_MaterialData.EmissiveColour.rgb * u_MaterialData.EmissiveColour.w;
	// an emissive map is its own colour (glTF: factor x map); a bare emissive colour glows in the albedo's tint
	if(u_MaterialData.EmissiveMapFactor >= 0.05)
		return emissive * DeGamma(texture(u_EmissiveMap, VertexOutput.TexCoord).rgb);
	return emissive * albedo;
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

// How far along the normal to push the shadow lookup, in shadow texels.
#define NORMAL_OFFSET_TEXELS 2.0

// Per-cascade world scale, recovered from the cascade matrix so this needs no
// extra uniforms. ShadowTransform is an orthographic projection times a rigid
// view transform, so each clip component's gradient with respect to world
// position is constant over the cascade:
//   |grad(clip.x)| = 1 / cascadeRadius   (clip x spans [-1,1] across 2*radius)
//   |grad(clip.z)| = 1 / (far - near)    (clip z spans [0,1] on Vulkan)
// Returns (world size of one shadow texel, depth units per world unit).
vec2 GetCascadeScale(int cascadeIndex, float shadowMapSize)
{
	mat4 m              = u_SceneData.ShadowTransform[cascadeIndex];
	float invRadius     = length(vec3(m[0].x, m[1].x, m[2].x));
	float depthPerWorld = length(vec3(m[0].z, m[1].z, m[2].z));
	return vec2((2.0 / shadowMapSize) / max(invRadius, 1e-8), depthPerWorld);
}

// InitialBias is measured in shadow texels. Converting through cascadeScale ties
// the offset to what a texel actually covers, so one value holds across every
// cascade - previously the bias was in the cascade's normalised depth units and
// was then scaled again by the cascade index, which stacked up to metres of
// offset in the far cascades and slid shadows off their casters.
float GetShadowBias(vec3 lightDirection, vec3 normal, vec2 cascadeScale)
{
	// A surface tilted away from the light spans more depth across one texel, so
	// scale by tan(theta). Clamped so grazing angles don't run away.
	float NoL   = clamp(dot(normal, lightDirection), 0.0, 1.0);
	float slope = clamp(sqrt(1.0 - NoL * NoL) / max(NoL, 0.05), 0.0, 4.0);

	float worldBias = cascadeScale.x * u_SceneData.InitialBias * (1.0 + slope);
	return worldBias * cascadeScale.y;
}

// Normal-offset bias: move the lookup across the surface rather than pushing
// depth along the light direction. This clears acne without sliding the shadow
// out from under its caster, which is what shows up as peter panning.
vec3 ApplyNormalOffset(vec3 wsPos, vec3 normal, vec3 lightDirection, float texelWorldSize)
{
	float NoL      = clamp(dot(normal, lightDirection), 0.0, 1.0);
	float sinTheta = sqrt(clamp(1.0 - NoL * NoL, 0.0, 1.0));
	return wsPos + normal * (texelWorldSize * NORMAL_OFFSET_TEXELS * sinTheta);
}

float InterleavedGradientNoise(vec2 screenPosition)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPosition, magic.xy)));
}

// One shadow tap, filtered: compares the four texels around uv and blends the
// results by where uv sits between them. A plain compare snaps to the nearest
// texel, which is what turns a shadow edge into a staircase.
float SampleShadowBilinear(sampler2DArray shadowMap, vec2 uv, int cascadeIndex, float compareZ, float bias)
{
	vec2 size = vec2(textureSize(shadowMap, 0).xy);
	vec2 f    = fract(uv * size - 0.5);
	// gather order: (0,1) (1,1) (1,0) (0,0)
	vec4 depths = textureGather(shadowMap, vec3(uv, cascadeIndex), 0) + bias;
	vec4 lit    = step(vec4(compareZ), depths);
	return mix(mix(lit.w, lit.z, f.x), mix(lit.x, lit.y, f.x), f.y);
}

float PCFShadowDirectionalLight(sampler2DArray shadowMap, vec4 shadowCoords, float uvRadius, float bias, vec3 wsPos, int cascadeIndex)
{
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

		vec2 sampleUV = clamp(shadowCoords.xy + offset, vec2(0.001), vec2(0.999));
		sum += SampleShadowBilinear(shadowMap, sampleUV, cascadeIndex, shadowCoords.z, bias);
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

	float shadowMapSize = float(textureSize(uShadowMap, 0).x);
	vec2 cascadeScale   = GetCascadeScale(cascadeIndex, shadowMapSize);
	vec3 samplePos      = ApplyNormalOffset(wsPos, normal, lightDirection, cascadeScale.x);

	vec4 shadowCoord = u_SceneData.BiasMatrix * u_SceneData.ShadowTransform[cascadeIndex] * vec4(samplePos, 1.0);
	shadowCoord = shadowCoord * (1.0 / shadowCoord.w);

	if(shadowCoord.z > 0.999 || shadowCoord.z < 0.0)
		return 1.0;

	if(shadowCoord.x < 0.0 || shadowCoord.x > 1.0 || shadowCoord.y < 0.0 || shadowCoord.y > 1.0)
		return 1.0;

	// Fade shadow near cascade edges to avoid hard borders
	float edgeFade = 1.0;
	float edgeMargin = 0.05;
	edgeFade *= smoothstep(0.0, edgeMargin, shadowCoord.x);
	edgeFade *= smoothstep(0.0, edgeMargin, 1.0 - shadowCoord.x);
	edgeFade *= smoothstep(0.0, edgeMargin, shadowCoord.y);
	edgeFade *= smoothstep(0.0, edgeMargin, 1.0 - shadowCoord.y);

	float shadowAmount = 1.0;

	float uvRadius = 0.002;
	float bias     = GetShadowBias(lightDirection, normal, cascadeScale);

	if (u_SceneData.FilterShadows  == 1)
	{
		float texelSize = 1.0 / shadowMapSize;
		uvRadius = u_SceneData.LightSize * texelSize * 2.0;
		uvRadius = clamp(uvRadius, texelSize, texelSize * 8.0);

		shadowAmount = PCFShadowDirectionalLight(uShadowMap, shadowCoord, uvRadius, bias, wsPos, cascadeIndex);
	}
	else
	{
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
			// The next cascade covers more world per texel, so it needs its own
			// normal offset and bias - reusing this cascade's would under-bias it.
			vec2 cascadeScaleNext = GetCascadeScale(cascadeNext, shadowMapSize);
			vec3 samplePosNext    = ApplyNormalOffset(wsPos, normal, lightDirection, cascadeScaleNext.x);
			float biasNext        = GetShadowBias(lightDirection, normal, cascadeScaleNext);

			vec4 shadowCoordNext = u_SceneData.BiasMatrix * u_SceneData.ShadowTransform[cascadeNext] * vec4(samplePosNext, 1.0);
			shadowCoordNext = shadowCoordNext * (1.0 / shadowCoordNext.w);

			float shadowAmount1;
			if (u_SceneData.FilterShadows == 1)
			{
				shadowAmount1 = PCFShadowDirectionalLight(uShadowMap, shadowCoordNext, uvRadius, biasNext, wsPos, cascadeNext);
			}
			else
			{
				float zNext = texture(uShadowMap, vec3(shadowCoordNext.xy, cascadeNext)).r;
				shadowAmount1 = step(shadowCoordNext.z - biasNext, zNext);
			}

			shadowAmount = mix(shadowAmount, shadowAmount1, cascadeFade);
		}
	}

	return 1.0 - ((1.0 - shadowAmount) * ShadowFade * edgeFade);
}


// Spot light shadow: a perspective map in one tile of the atlas. Bias and normal offset are in
// texels, and a texel grows with the distance from the light.
float CalculateSpotShadow(int slot, vec3 wsPos, vec3 lightPos, vec3 toLight, vec3 normal)
{
	vec4 rect   = u_SceneData.SpotShadowRect[slot];
	vec4 params = u_SceneData.SpotShadowParams[slot];

	float dist       = length(lightPos - wsPos);
	float texelWorld = params.z * dist;
	float NoL        = clamp(dot(normal, toLight), 0.0, 1.0);
	float sinTheta   = sqrt(clamp(1.0 - NoL * NoL, 0.0, 1.0));
	vec3 samplePos   = wsPos + normal * (texelWorld * params.y * NORMAL_OFFSET_TEXELS * sinTheta);

	vec4 coord = u_SceneData.SpotShadowMatrix[slot] * vec4(samplePos, 1.0);
	if(coord.w <= 0.0)
		return 1.0;
	coord.xyz /= coord.w;
	if(coord.z >= 1.0 || coord.z <= 0.0 || any(lessThan(coord.xy, vec2(0.0))) || any(greaterThan(coord.xy, vec2(1.0))))
		return 1.0;

	// depth is non-linear here: turn the world-space bias into a depth step through the slope of z
	float slope   = clamp(sqrt(1.0 - NoL * NoL) / max(NoL, 0.05), 0.0, 4.0);
	float biasW   = texelWorld * params.x * (1.0 + slope);
	vec4 coordB   = u_SceneData.SpotShadowMatrix[slot] * vec4(samplePos + toLight * biasW, 1.0);
	float bias    = max(coord.z - coordB.z / coordB.w, 0.0);

	float layerSize = float(textureSize(uShadowMap, 0).x);
	float tileTexel = 1.0 / (rect.z * layerSize); // one texel, in tile uv
	vec2 lo = vec2(tileTexel * 1.5);
	vec2 hi = vec2(1.0) - lo;
	int layer = int(rect.w);

	float lit = 0.0;
	if(u_SceneData.FilterShadows == 1)
	{
		float uvRadius = clamp(u_SceneData.LightSize * tileTexel * 1.5, tileTexel, tileTexel * 6.0);
		int sampleCount = min(u_SceneData.PCFSamples, 16);
		float noise = InterleavedGradientNoise(gl_FragCoord.xy) * TwoPI;
		for(int i = 0; i < sampleCount; i++)
		{
			vec2 offset = VogelDiskSample(i, InvSqrtPCFSamples, noise) * uvRadius;
			vec2 uv     = rect.xy + clamp(coord.xy + offset, lo, hi) * rect.z;
			lit += SampleShadowBilinear(uShadowMap, uv, layer, coord.z, bias);
		}
		lit /= float(sampleCount);
	}
	else
	{
		vec2 uv = rect.xy + clamp(coord.xy, lo, hi) * rect.z;
		lit = step(coord.z - bias, texture(uShadowMap, vec3(uv, layer)).r);
	}

	return 1.0 - (1.0 - lit) * params.w;
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

#ifdef FORWARD_PLUS
	ivec2 tileID = ivec2(gl_FragCoord.xy) / u_SceneData.TileSize;
	uint tileIdx = uint(tileID.y) * uint(u_SceneData.TileCountX) + uint(tileID.x);
	uint tileOffset = u_LightGrid.tiles[tileIdx].x;
	uint tileCount  = u_LightGrid.tiles[tileIdx].y;

	for(uint li = 0; li < tileCount; li++)
	{
		Light light = u_Lights.lights[u_LightIndices.indices[tileOffset + li]];
#else
	for(int i = 0; i < u_SceneData.LightCount; i++)
	{
		Light light = u_SceneData.lights[i];
#endif
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

			// light range mode: Radius is a hard range, fade to nothing at it
			if(u_SceneData.ScenePad0 > 0.5)
			{
				float q = dist / light.radius;
				q *= q;
				float w = clamp(1.0 - q * q, 0.0, 1.0);
				attenuation *= w * w;
			}
			value = clamp(attenuation, 0.0, 1.0);

			int shadowSlot = int(light.direction.w);
			if(shadowSlot >= 0 && value > 0.0 && u_SceneData.ShadowEnabled > 0)
				value *= CalculateSpotShadow(shadowSlot, wsPos, light.position.xyz, L, GeometricNormal);
		}
		else
		{
			float nDotL = dot(material.Normal, light.direction.xyz);

			if(u_SceneData.ShadowEnabled > 0 && nDotL > 0.0f)
			{
				int cascadeIndex = CalculateCascadeIndex(wsPos);
				value = CalculateShadow(wsPos,cascadeIndex, light.direction.xyz, GeometricNormal);
			}
			else
				value = 1.0;
			// screen-space contacts are for the sun (the first directional light) only
			value *= ContactShadow;
		}


		vec3 Li = light.direction.xyz;
		float lightNoL = saturate(dot(material.Normal, Li));
		// out of range / outside the cone / facing away: nothing to add, skip the BRDF
		if(value <= 0.0 || lightNoL <= 0.0)
			continue;
		vec3 Lradiance = light.colour.xyz * light.intensity;
#if QUALITY_LOW
		// below about 1/255 of the picture: not worth the BRDF
		if(value * max3(Lradiance) < 0.004)
			continue;
#endif
		vec3 h = normalize(material.View + Li);

		float shading_NoV = clampNoV(dot(material.Normal, material.View));
    	float NoV = shading_NoV;
    	float NoL = saturate(lightNoL);
    	float NoH = saturate(dot(material.Normal, h));
    	float LoH = saturate(dot(Li, h));

    	vec3 Fd = DiffuseLobe(material, NoV, NoL, LoH);
		vec3 Fr = SpecularLobe(material, light, h, NoV, NoL, NoH, LoH);
		// Clamp spec contribution to prevent fireflies from point-light sub-pixel alignment
		Fr = min(Fr * material.EnergyCompensation, vec3(20.0));

		vec3 colour = Fd + Fr;

		result += (colour * Lradiance.rgb) * (value * NoL * ComputeMicroShadowing(NoL, material.AO));
	}
	return result;
}

vec4 GIIrradiance(vec3 wsPos, vec3 n)
{
	if(u_SceneData.GIOrigin.w < 0.5)
		return vec4(0.0);
	vec4 gi = GISampleGrid(wsPos, n, u_SceneData.GIOrigin.xyz, u_SceneData.GISpacing.xyz, u_SceneData.GICounts.xyz);
	gi.rgb *= u_SceneData.GISpacing.w;
	return gi;
}

vec3 IBL(vec3 F0, vec3 Lr, Material material, vec3 wsPos, float ssao)
{
	vec3 irradiance = texture(uIrrMap, material.Normal).rgb;

	// the probes know what is around; the sky map alone lights a room as if it had no walls
	float specularOcclusion = 1.0;
	float giWeight          = 0.0;
	vec4 gi = GIIrradiance(wsPos, material.Normal);
	if(gi.w > 0.0)
	{
		float skyLum = max(dot(irradiance, vec3(0.2126, 0.7152, 0.0722)), 1e-6);
		float giLum  = dot(gi.rgb, vec3(0.2126, 0.7152, 0.0722));
		// the irradiance ratio says roughly how much sky the surface sees; reflections never go fully dark from it
		specularOcclusion = mix(1.0, max(sqrt(clamp(giLum / skyLum, 0.0, 1.0)), 0.3), gi.w);
		irradiance        = mix(irradiance, gi.rgb, gi.w);
		giWeight          = gi.w;
	}

	// the probes already hold the large-scale occlusion: SSAO only adds the fine part on top
	float diffuseAO = material.AO * mix(ssao, sqrt(ssao), giWeight);
	// Lagarde & de Rousiers 2014: specular occlusion from AO, tighter for smooth surfaces
	float ao     = material.AO * ssao;
	float specAO = clamp(pow(material.NDotV + ao, exp2(-16.0 * material.Roughness - 1.0)) - 1.0 + ao, 0.0, 1.0);

	vec3 E = F0 * material.dfg.x + material.dfg.y;
	vec3 kd = (1.0 - E) * (1.0 - material.Metallic.x);
	vec3 diffuseIBL = material.Albedo.xyz * irradiance;

	float maxMipLevel = max(float(u_SceneData.EnvMipCount) - 1.0, 1.0);
	vec3 specularIrradiance = textureLod(uEnvMap, Lr, material.PerceptualRoughness * maxMipLevel).rgb;
	// Cap extreme HDR pixels (sun) to prevent fireflies on near-mirror surfaces
	specularIrradiance = min(specularIrradiance, vec3(50.0));

	vec3 specularIBL = specularIrradiance * E * material.EnergyCompensation * specularOcclusion;

	return (kd * diffuseIBL * diffuseAO + specularIBL * specAO) * u_SceneData.EnvExposure;
}

vec4 SampleDecalTexture(int slot, vec2 uv)
{
	// a sampler array can only be indexed with a constant on some drivers
	switch(slot)
	{
		case 0: return texture(uDecalTextures[0], uv);
		case 1: return texture(uDecalTextures[1], uv);
		case 2: return texture(uDecalTextures[2], uv);
		case 3: return texture(uDecalTextures[3], uv);
	}
	return vec4(1.0);
}

// Projected box decals, blended into the surface before lighting so they are lit and shadowed with it
void ApplyDecals(vec3 wsPos, vec3 n, inout vec4 albedo, inout float roughness, inout vec3 emissive)
{
	int count = int(u_SceneData.SceneExtras.y);
	for(int i = 0; i < count; i++)
	{
		vec3 local = (u_Decals.decals[i].WorldToDecal * vec4(wsPos, 1.0)).xyz;
		if(any(greaterThan(abs(local), vec3(1.0))))
			continue;

		// faces turned away from the projection get none of it, steep ones fade
		float facing = dot(n, u_Decals.decals[i].Axis.xyz);
		float fadeStart = u_Decals.decals[i].Params.y;
		float angleFade = clamp((facing - 0.05) / max(fadeStart - 0.05, 1e-3), 0.0, 1.0);
		// and the top / bottom of the box fade too, so a decal never ends in a hard cut
		float depthFade = 1.0 - smoothstep(0.8, 1.0, abs(local.y));
		if(angleFade * depthFade <= 0.0)
			continue;

		vec2 uv = local.xz * 0.5 + 0.5;
		uv.y    = 1.0 - uv.y;
		vec4 c  = u_Decals.decals[i].Colour;
		int slot = int(u_Decals.decals[i].Params.x);
		if(slot >= 0)
		{
			vec4 t = SampleDecalTexture(slot, uv);
			c *= vec4(DeGamma(t.rgb), t.a);
		}

		float a = c.a * angleFade * depthFade;
		albedo.rgb = mix(albedo.rgb, c.rgb, a);
		if(u_Decals.decals[i].Params.z >= 0.0)
			roughness = mix(roughness, u_Decals.decals[i].Params.z, a);
		emissive += c.rgb * u_Decals.decals[i].Params.w * a;
	}
}

void main()
{
	vec4 texColour = GetAlbedo();
	if(texColour.w < u_MaterialData.AlphaCutOff)
		discard;
	vec3 decalEmissive = vec3(0.0);

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
		vec4 tex        = texture(u_MetallicMap, VertexOutput.TexCoord);
		vec3 specular   = (1.0 - u_MaterialData.MetallicMapFactor) * vec3(u_MaterialData.Metallic)
		                + u_MaterialData.MetallicMapFactor * tex.rgb;
		float glossiness = (1.0 - u_MaterialData.MetallicMapFactor) * u_MaterialData.Roughness
		                 + u_MaterialData.MetallicMapFactor * tex.a;
		metallic  = computeMetallicFromSpecularColour(specular);
		roughness = 1.0 - glossiness;
	}

	ApplyDecals(VertexOutput.Position.xyz, normalize(VertexOutput.Normal), texColour, roughness, decalEmissive);

	Material material;
    material.Albedo    = texColour;
    material.Metallic  = metallic;
    material.PerceptualRoughness = roughness;
	material.Reflectance = u_MaterialData.Reflectance;
	material.Normal = normalize(VertexOutput.Normal);
	GeometricNormal = material.Normal;

	if (u_MaterialData.NormalMapFactor > 0.04)
	{
		material.Normal = normalize(texture(u_NormalMap, VertexOutput.TexCoord).rgb * 2.0f - 1.0f);
		material.Normal = normalize(VertexOutput.WorldNormal * material.Normal);
	}

	material.AO		   = GetAO();
	material.Emissive  = GetEmissive(material.Albedo.rgb) + decalEmissive;

	vec2 uv = gl_FragCoord.xy / vec2(u_SceneData.Width, u_SceneData.Height);
	float ssao = texture(uSSAOMap, uv).r;
	if(u_SceneData.SceneExtras.x > 0.5)
		ContactShadow = texture(uContactShadowMap, uv).r;

    // Start from clamped perceptual
	material.PerceptualRoughness = clamp(material.PerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
	float roughness2 = material.PerceptualRoughness * material.PerceptualRoughness;

	// Specular anti-aliasing
#if !QUALITY_LOW
	{
		const float strength = 1.0;
		const float maxRoughnessGain = 0.18;

		vec3 dndu = dFdx(material.Normal);
		vec3 dndv = dFdy(material.Normal);
		float variance = dot(dndu, dndu) + dot(dndv, dndv);

		float kernelRoughness2 = min(variance * strength, maxRoughnessGain);

		float filteredRoughness2 = roughness2 + kernelRoughness2;
		filteredRoughness2 = clamp(filteredRoughness2, MIN_ROUGHNESS, 1.0);

		material.Roughness = filteredRoughness2;
	}
#else
	material.Roughness = roughness2;
#endif

	InvSqrtPCFSamples = 1.0 / sqrt(float(u_SceneData.PCFSamples));

	vec3 wsPos     = VertexOutput.Position.xyz;
	material.View  = normalize(u_SceneData.cameraPosition.xyz - wsPos);
	material.NDotV = max(dot(material.Normal, material.View), 1e-4);

	material.dfg = texture(uBRDFLUT, vec2(material.NDotV, material.PerceptualRoughness)).rg;
	float reflectance = computeDielectricF0(material.Reflectance);
	vec3 F0 = computeF0(material.Albedo, material.Metallic.x, reflectance);
	material.F0 = F0;
    // multi-scatter: the LUT is (scale, bias), so x + y = what one bounce reflects at F0 = 1
    material.EnergyCompensation = 1.0 + material.F0 * (1.0 / max(material.dfg.x + material.dfg.y, 0.05) - 1.0);
	material.Albedo.xyz = computeDiffuseColour(material.Albedo, material.Metallic.x);

	vec3 Lr = 2.0 * material.NDotV * material.Normal - material.View;
	vec3 lightContribution = Lighting(material.F0, wsPos, material);
	vec3 iblContribution   = IBL(material.F0, Lr, material, wsPos, ssao);

	vec3 finalColour = lightContribution + iblContribution + material.Emissive;

	if(u_SceneData.FogColour.a > 0.001)
	{
		vec3 camPos = u_SceneData.cameraPosition.xyz;
		float dist = length(wsPos - camPos);

		// Exponential, height-attenuated fog (Wronski-style).
		float density = u_SceneData.FogParams.x;
		float heightFall = u_SceneData.FogParams.y;
		float fogExp = 0.0;
		if(density > 0.0)
		{
			float yCam = camPos.y;
			float yPos = wsPos.y;
			float dy = yPos - yCam;
			// Avoid div-by-zero when looking horizontally.
			float falloffSafe = (heightFall > 0.0001) ? heightFall : 0.0001;
			float verticalAtt = (heightFall > 0.0001)
				? (1.0 - exp(-dy * falloffSafe)) / (dy * falloffSafe + 1e-5)
				: 1.0;
			fogExp = 1.0 - exp(-density * dist * verticalAtt);
			fogExp = clamp(fogExp, 0.0, 1.0);
		}

		// Linear distance band — useful for distant haze without crushing nearby props.
		float startD = u_SceneData.FogParams.z;
		float endD   = u_SceneData.FogParams.w;
		float fogLin = 0.0;
		if(endD > startD + 0.001)
			fogLin = clamp((dist - startD) / (endD - startD), 0.0, 1.0);

		float fog = max(fogExp, fogLin) * u_SceneData.FogColour.a;
		finalColour = mix(finalColour, u_SceneData.FogColour.rgb, fog);
	}

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
			case 8:
			outColour = vec4(VertexOutput.TexCoord, 0.0, 1.0);
			break;
			case 9:
			{
				// what the GI probes light this surface with (bright = more bounce light)
				vec4 gi = GIIrradiance(wsPos, material.Normal);
				outColour = vec4(gi.rgb * gi.w * u_SceneData.EnvExposure, 1.0);
				break;
			}
		}
	}
}
