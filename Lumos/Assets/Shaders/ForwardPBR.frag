#version 450
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

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 4
#define BLEND_SHADOW_CASCADES 1
#define FILTER_SHADOWS 1
#define NUM_PCF_SAMPLES 8
float ShadowFade = 1.0;

struct Light
{
	vec4 colour;
	vec4 position;
	vec4 direction;
	float intensity;
	float radius;
	float type;
	float angle;
};

layout(set = 1, binding = 0) uniform sampler2D u_AlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D u_MetallicMap;
layout(set = 1, binding = 2) uniform sampler2D u_RoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D u_NormalMap;
layout(set = 1, binding = 4) uniform sampler2D u_AOMap;
layout(set = 1, binding = 5) uniform sampler2D u_EmissiveMap;

layout(set = 1,binding = 6) uniform UniformMaterialData
{
	vec4  AlbedoColour;
	float Roughness;
	float Metallic;
	float Reflectance;
	float Emissive;
	float AlbedoMapFactor;
	float MetallicMapFactor;
	float RoughnessMapFactor;
	float NormalMapFactor;
	float EmissiveMapFactor;
	float AOMapFactor;
	float AlphaCutOff;
	float workflow;
} materialProperties;

layout(set = 2, binding = 0) uniform sampler2D uBRDFLUT;
layout(set = 2, binding = 1) uniform samplerCube uEnvMap;
layout(set = 2, binding = 2) uniform samplerCube uIrrMap;
layout(set = 2, binding = 3) uniform sampler2DArray uShadowMap;
layout(set = 2, binding = 4) uniform sampler2D uSSAOMap;

layout(set = 2, binding = 5) uniform UBOLight
{
	Light lights[MAX_LIGHTS];
	mat4 ShadowTransform[MAX_SHADOWMAPS];
	mat4 ViewMatrix;
	mat4 LightView;
	mat4 BiasMatrix;
	vec4 cameraPosition;
	vec4 SplitDepths[MAX_SHADOWMAPS];
	float LightSize;
	float MaxShadowDist;
	float ShadowFade;
	float CascadeFade;
	int LightCount;
	int ShadowCount;
	int Mode;
	int EnvMipCount;
	float InitialBias;
	float Width;
	float Height;
	int shadowEnabled;
} ubo;

layout(location = 0) out vec4 outColor;

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
	if(materialProperties.AlbedoMapFactor < 0.05)
		return  materialProperties.AlbedoColour;

	return (1.0 - materialProperties.AlbedoMapFactor) * materialProperties.AlbedoColour + materialProperties.AlbedoMapFactor * DeGamma(texture(u_AlbedoMap, VertexOutput.TexCoord));
}

vec3 GetMetallic()
{
	if(materialProperties.MetallicMapFactor < 0.05)
		return  materialProperties.Metallic.rrr;

	return (1.0 - materialProperties.MetallicMapFactor) * materialProperties.Metallic + materialProperties.MetallicMapFactor * texture(u_MetallicMap, VertexOutput.TexCoord).rgb;
}

float GetRoughness()
{
	if(materialProperties.RoughnessMapFactor < 0.05)
		return  materialProperties.Roughness;
	return (1.0 - materialProperties.RoughnessMapFactor) * materialProperties.Roughness + materialProperties.RoughnessMapFactor * texture(u_RoughnessMap, VertexOutput.TexCoord).r;
}

float GetAO()
{
	if(materialProperties.AOMapFactor < 0.05)
		return 1.0;

	return (1.0 - materialProperties.AOMapFactor) + materialProperties.AOMapFactor * texture(u_AOMap, VertexOutput.TexCoord).r;
}

vec3 GetEmissive(vec3 albedo)
{
	if(materialProperties.EmissiveMapFactor < 0.05)
		return (materialProperties.Emissive * albedo);
	return (materialProperties.Emissive * albedo) + materialProperties.EmissiveMapFactor * DeGamma(texture(u_EmissiveMap, VertexOutput.TexCoord).rgb);
}

vec3 GetNormalFromMap()
{
	if (materialProperties.NormalMapFactor < 0.05)
		return normalize(VertexOutput.Normal);

	vec3 Normal = normalize(texture(u_NormalMap, VertexOutput.TexCoord).rgb * 2.0f - 1.0f);
	return normalize(VertexOutput.WorldNormal * Normal);
}

const mat4 BiasMatrix = mat4(
						  0.5, 0.0, 0.0, 0.5,
						  0.0, 0.5, 0.0, 0.5,
						  0.0, 0.0, 1.0, 0.0,
						  0.0, 0.0, 0.0, 1.0
						  );

const vec2 PoissonDistribution16[16] = vec2[](
											  vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725), vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
											  vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464), vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
											  vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420), vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
											  vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590), vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
											  );


const vec2 PoissonDistribution[64] = vec2[](
											vec2(-0.884081, 0.124488), vec2(-0.714377, 0.027940), vec2(-0.747945, 0.227922), vec2(-0.939609, 0.243634),
											vec2(-0.985465, 0.045534),vec2(-0.861367, -0.136222),vec2(-0.881934, 0.396908),vec2(-0.466938, 0.014526),
											vec2(-0.558207, 0.212662),vec2(-0.578447, -0.095822),vec2(-0.740266, -0.095631),vec2(-0.751681, 0.472604),
											vec2(-0.553147, -0.243177),vec2(-0.674762, -0.330730),vec2(-0.402765, -0.122087),vec2(-0.319776, -0.312166),
											vec2(-0.413923, -0.439757),vec2(-0.979153, -0.201245),vec2(-0.865579, -0.288695),vec2(-0.243704, -0.186378),
											vec2(-0.294920, -0.055748),vec2(-0.604452, -0.544251),vec2(-0.418056, -0.587679),vec2(-0.549156, -0.415877),
											vec2(-0.238080, -0.611761),vec2(-0.267004, -0.459702),vec2(-0.100006, -0.229116),vec2(-0.101928, -0.380382),
											vec2(-0.681467, -0.700773),vec2(-0.763488, -0.543386),vec2(-0.549030, -0.750749),vec2(-0.809045, -0.408738),
											vec2(-0.388134, -0.773448),vec2(-0.429392, -0.894892),vec2(-0.131597, 0.065058),vec2(-0.275002, 0.102922),
											vec2(-0.106117, -0.068327),vec2(-0.294586, -0.891515),vec2(-0.629418, 0.379387),vec2(-0.407257, 0.339748),
											vec2(0.071650, -0.384284),vec2(0.022018, -0.263793),vec2(0.003879, -0.136073),vec2(-0.137533, -0.767844),
											vec2(-0.050874, -0.906068),vec2(0.114133, -0.070053),vec2(0.163314, -0.217231),vec2(-0.100262, -0.587992),
											vec2(-0.004942, 0.125368),vec2(0.035302, -0.619310),vec2(0.195646, -0.459022),vec2(0.303969, -0.346362),
											vec2(-0.678118, 0.685099),vec2(-0.628418, 0.507978),vec2(-0.508473, 0.458753),vec2(0.032134, -0.782030),
											vec2(0.122595, 0.280353),vec2(-0.043643, 0.312119),vec2(0.132993, 0.085170),vec2(-0.192106, 0.285848),
											vec2(0.183621, -0.713242),vec2(0.265220, -0.596716),vec2(-0.009628, -0.483058),vec2(-0.018516, 0.435703)
											);


vec2 SamplePoisson(int index)
{
	return PoissonDistribution[index % 64];
}

vec2 SamplePoisson16(int index)
{
	return PoissonDistribution16[index % 16];
}

float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio   

float GoldNoise(vec2 xy, float seed)
{
	return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

float Noise(vec2 co)
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float rand(vec2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

vec2 VogelDiskSample(int sampleIndex, int samplesCount, float phi)
{
  float GoldenAngle = 2.4f;

  float r = sqrt(sampleIndex + 0.5f) / sqrt(samplesCount);
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

vec2 SearchRegionRadiusUV(float zWorld)
{
	float light_zNear = 0.0; 
	vec2 lightRadiusUV = vec2(0.05);
    return lightRadiusUV * (zWorld - light_zNear) / zWorld;
}

float GetShadowBias(vec3 lightDirection, vec3 normal, int shadowIndex)
{
	float minBias = ubo.InitialBias;
	float bias = max(minBias * (1.0 - dot(normal, lightDirection)), minBias);
	return bias;
}

float PCFShadowDirectionalLight(sampler2DArray shadowMap, vec4 shadowCoords, float uvRadius, vec3 lightDirection, vec3 normal, vec3 wsPos, int cascadeIndex)
{
	float bias = GetShadowBias(lightDirection, normal, cascadeIndex);
	float sum = 0;
	float noise = Noise(wsPos.xy);

	for (int i = 0; i < NUM_PCF_SAMPLES; i++)
	{
		//int index = int(16.0f*Random(vec4(wsPos, i)))%16;
		//int index = int(float(NUM_PCF_SAMPLES)*GoldNoise(wsPos.xy, i * wsPos.z))%NUM_PCF_SAMPLES;
		//int index = int(float(NUM_PCF_SAMPLES)*Random(vec4(wsPos.xyz, 1)))%NUM_PCF_SAMPLES;
		//int index = int(float(NUM_PCF_SAMPLES)*Random(vec4(floor(wsPos*1000.0), 1)))%NUM_PCF_SAMPLES;
		//int index = int(NUM_PCF_SAMPLES*Random(vec4(floor(wsPos.xyz*1000.0), i)))%NUM_PCF_SAMPLES;
		
		//int index = int(NUM_PCF_SAMPLES*Random(vec4(wsPos.xyy, i)))%NUM_PCF_SAMPLES;
		//vec2 offset = (SamplePoisson(index) / 700.0f);
		vec2 offset = VogelDiskSample(i, NUM_PCF_SAMPLES, noise) / 700.0f;
		
		float z = texture(shadowMap, vec3(shadowCoords.xy + offset, cascadeIndex)).r;
		sum += step(shadowCoords.z - bias, z);
	}
	
	return sum / NUM_PCF_SAMPLES;
}

int CalculateCascadeIndex(vec3 wsPos)
{
	int cascadeIndex = 0;
	vec4 viewPos = ubo.ViewMatrix * vec4(wsPos, 1.0);
	
	for(int i = 0; i < ubo.ShadowCount - 1; ++i)
	{
		if(viewPos.z < ubo.SplitDepths[i].x)
		{
			cascadeIndex = i + 1;
		}
	}
	
	return cascadeIndex;
}

float CalculateShadow(vec3 wsPos, int cascadeIndex, vec3 lightDirection, vec3 normal)
{
	vec4 shadowCoord = ubo.BiasMatrix * ubo.ShadowTransform[cascadeIndex] * vec4(wsPos, 1.0);
	shadowCoord = shadowCoord * (1.0 / shadowCoord.w);
	float NEAR = 0.01;
	float uvRadius =  ubo.LightSize * NEAR / shadowCoord.z;
	uvRadius = min(uvRadius, 0.002f);
	vec4 viewPos = ubo.ViewMatrix * vec4(wsPos, 1.0);
	
	float shadowAmount = 1.0;
	
#if (FILTER_SHADOWS  == 1)
	shadowAmount = PCFShadowDirectionalLight(uShadowMap, shadowCoord, uvRadius, lightDirection, normal, wsPos, cascadeIndex);
#else
	float bias = GetShadowBias(lightDirection, normal, cascadeIndex);
	float z = texture(uShadowMap, vec3(shadowCoord.xy, cascadeIndex)).r;
	shadowAmount = step(shadowCoord.z - bias, z);
#endif
	
#if (BLEND_SHADOW_CASCADES == 1)
	float cascadeFade = smoothstep(ubo.SplitDepths[cascadeIndex].x + ubo.CascadeFade, ubo.SplitDepths[cascadeIndex].x, viewPos.z);
	int cascadeNext = cascadeIndex + 1;
	if (cascadeFade > 0.0 && cascadeNext < ubo.ShadowCount)
	{
		shadowCoord = ubo.BiasMatrix * ubo.ShadowTransform[cascadeNext] * vec4(wsPos, 1.0);
		float shadowAmount1 = PCFShadowDirectionalLight(uShadowMap, shadowCoord, uvRadius, lightDirection, normal, wsPos, cascadeNext);
		
		shadowAmount =  mix(shadowAmount, shadowAmount1, cascadeFade);
	}
	#endif
	
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

#define NEW_LIGHTING 1

vec3 Lighting(vec3 F0, vec3 wsPos, Material material)
{
	vec3 result = vec3(0.0);
	
	for(int i = 0; i < ubo.LightCount; i++)
	{
		Light light = ubo.lights[i];
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
			float atten = light.radius / (pow(dist, 2.0) + 1.0);
			float attenuation = clamp(1.0 - (dist * dist) / (light.radius * light.radius), 0.0, 1.0);

			value = attenuation;
			
			light.direction = vec4(L,1.0);
		}
		else if (light.type == 1.0)
		{
			vec3 L = light.position.xyz - wsPos;
			float cutoffAngle   = 1.0f - light.angle;      
			float dist          = length(L);
			L = normalize(L);
			float theta         = dot(L.xyz, light.direction.xyz);
			float epsilon       = cutoffAngle - cutoffAngle * 0.9f;
			float attenuation 	= ((theta - cutoffAngle) / epsilon); // atteunate when approaching the outer cone
			attenuation         *= light.radius / (pow(dist, 2.0) + 1.0);//saturate(1.0f - dist / light.range);
			//float intensity 	= attenuation * attenuation;
			
			// Erase light if there is no need to compute it
			//intensity *= step(theta, cutoffAngle);
			
			value = clamp(attenuation, 0.0, 1.0);
		}
		else
		{
			int cascadeIndex = CalculateCascadeIndex(wsPos);
			if(ubo.shadowEnabled > 0)
				value = CalculateShadow(wsPos,cascadeIndex, light.direction.xyz, material.Normal);
			else
				value = 1.0;
		}
		
		vec3 Li = light.direction.xyz;
		vec3 Lradiance = light.colour.xyz * light.intensity;
		vec3 Lh = normalize(Li + material.View);

		#ifndef NEW_LIGHTING
		
		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(material.Normal, Li));
		float cosLh = max(0.0, dot(material.Normal, Lh));
		
		vec3 F = fresnelSchlickRoughness(F0, max(0.0, dot(Lh,  material.View)), material.Roughness);
		
		float D = ndfGGX(cosLh, material.Roughness);
		float G = gaSchlickGGX(cosLi, material.NDotV, material.Roughness);
		
		vec3 kd = (1.0 - F) * (1.0 - material.Metallic.x);
		vec3 diffuseBRDF = kd * material.Albedo.xyz;
		
		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * material.NDotV);
		
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));//;
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi * value * ComputeMicroShadowing(saturate(cosLi), material.AO);

#else
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
#endif
	}
	return result;
}

vec3 IBL(vec3 F0, vec3 Lr, Material material)
{
	vec3 irradiance = texture(uIrrMap, material.Normal).rgb;
	vec3 F = fresnelSchlickRoughness(F0, material.NDotV, material.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - material.Metallic.x);
	vec3 diffuseIBL = material.Albedo.xyz * irradiance;
	
	int u_EnvRadianceTexLevels = ubo.EnvMipCount;// textureQueryLevels(uBRDFLUT);	
	vec3 specularIrradiance = textureLod(uEnvMap, Lr, material.PerceptualRoughness * u_EnvRadianceTexLevels).rgb;
	
	vec3 specularIBL = specularIrradiance * (F * material.dfg.x + material.dfg.y);
	
	return kd * diffuseIBL + specularIBL;
}

vec3 IBLNew(vec3 F0, vec3 Lr, Material material)
{
	  // specular layer
    vec3 Fr = vec3(0.0);

    vec3 E = mix(material.dfg.xxx, material.dfg.yyy, material.F0); //specularDFG(pixel);
    vec3 r = Lr;//getReflectedVector(pixel, material.Normal);

	int u_EnvRadianceTexLevels = ubo.EnvMipCount;	
	material.Roughness * u_EnvRadianceTexLevels;
	vec3 specularIrradiance = textureLod(uEnvMap, Lr, material.PerceptualRoughness * u_EnvRadianceTexLevels).rgb;
	//specularIrradiance = DeGamma(specularIrradiance);

    Fr = E * specularIrradiance;

	vec3 irradiance = texture(uIrrMap, material.Normal).rgb;
	//irradiance = DeGamma(irradiance);
    
    //vec3 diffuseIrradiance = diffuseIrradiance(shading_normal);
    vec3 Fd = material.Albedo.xyz * irradiance * (1.0 - E);// * diffuseBRDF;

    vec3 color = Fr + Fd;
	return color;
}

void main() 
{
	vec4 texColour = GetAlbedo();
	if(texColour.w < materialProperties.AlphaCutOff)
		discard;
	
	float metallic  = 0.0;
	float roughness = 0.0;
	
	if(materialProperties.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		metallic  = GetMetallic().x;
		roughness = GetRoughness();
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex  = texture(u_MetallicMap, VertexOutput.TexCoord).rgb;
		metallic  = (1.0 - materialProperties.MetallicMapFactor) * materialProperties.Metallic + materialProperties.MetallicMapFactor * tex.b;
		roughness = (1.0 - materialProperties.MetallicMapFactor) * materialProperties.Roughness + materialProperties.MetallicMapFactor * tex.g;
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		//TODO
		vec3 tex  = texture(u_MetallicMap, VertexOutput.TexCoord).rgb;
		metallic  = (1.0 - materialProperties.MetallicMapFactor) * materialProperties.Metallic + materialProperties.MetallicMapFactor * tex.b;
		roughness = (1.0 - materialProperties.MetallicMapFactor) * materialProperties.Roughness + materialProperties.MetallicMapFactor * tex.g;
	}
	
	Material material;
    material.Albedo    = texColour;
    material.Metallic  = metallic;
    material.PerceptualRoughness = roughness;
	material.Reflectance = materialProperties.Reflectance;
	material.Normal = normalize(VertexOutput.Normal);
	
	if (materialProperties.NormalMapFactor > 0.04)
	{
		material.Normal = normalize(texture(u_NormalMap, VertexOutput.TexCoord).rgb * 2.0f - 1.0f);
		material.Normal = normalize(VertexOutput.WorldNormal * material.Normal);
		material.Normal = normalize(material.Normal);
	}

	material.AO		   = GetAO();
	material.Emissive  = GetEmissive(material.Albedo.rgb);

	vec2 uv = gl_FragCoord.xy / vec2(ubo.Width, ubo.Height);
	float ssao = texture(uSSAOMap, uv).r;
	material.Albedo *= ssao;

    material.PerceptualRoughness = clamp(material.PerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
	material.Roughness = perceptualRoughnessToRoughness(material.Roughness);

    // Specular anti-aliasing
    {
        const float strength          	= 1.0f;
        const float maxRoughnessGain  	= 0.02f;
        float roughness2         		= roughness * roughness;
        vec3 dndu                		= dFdx(material.Normal);
	    vec3 dndv 				 		= dFdy(material.Normal);
        float variance           		= (dot(dndu, dndu) + dot(dndv, dndv));
        float kernelRoughness2   		= min(variance * strength, maxRoughnessGain);
        float filteredRoughness2 		= saturate(roughness2 + kernelRoughness2);
        material.Roughness       		= sqrt(filteredRoughness2);
    }

	material.Roughness = clamp(material.Roughness, MIN_ROUGHNESS, 1.0);

	vec3 wsPos     = VertexOutput.Position.xyz;
	material.View  = normalize(ubo.cameraPosition.xyz - wsPos);
	material.NDotV = max(dot(material.Normal, material.View), 1e-4);

	material.dfg = texture(uBRDFLUT, vec2(material.NDotV, material.PerceptualRoughness)).rg;
	float reflectance = computeDielectricF0(material.Reflectance);
	//vec3 F0 = mix(Fdielectric, material.Albedo.xyz, material.Metallic.x);
	vec3 F0 = computeF0(material.Albedo, material.Metallic.x, reflectance);
	material.F0 = F0;
    material.EnergyCompensation = 1.0 + material.F0 * (1.0 / max(0.1, material.dfg.y) - 1.0);
	material.Albedo.xyz = computeDiffuseColor(material.Albedo, material.Metallic.x);

    float shadowDistance     = ubo.MaxShadowDist;
	float transitionDistance = ubo.ShadowFade;
	
	vec4 viewPos = ubo.ViewMatrix * vec4(wsPos, 1.0);
	
	float distance = length(viewPos);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0);
	
	vec3 Lr = 2.0 * material.NDotV * material.Normal - material.View;	
	vec3 lightContribution = Lighting(material.F0, wsPos, material);
	vec3 iblContribution   = IBL(material.F0, Lr, material);
	
	vec3 finalColour = lightContribution + iblContribution + material.Emissive;
	outColor = vec4(finalColour, 1.0);
	
	if(ubo.Mode > 0)
	{
		switch(ubo.Mode)
		{
			case 1:
			outColor = material.Albedo;
			break;
			case 2:
			outColor = vec4(material.Metallic.rrr, 1.0);
			break;
			case 3:
			outColor = vec4(material.PerceptualRoughness.xxx,1.0);
			break;
			case 4:
			outColor = vec4(material.AO.xxx, 1.0);
			break;
			case 5:
			outColor = vec4(material.Emissive, 1.0);
			break;
			case 6:
			outColor = vec4(material.Normal,1.0);
			break;
            case 7:
			int cascadeIndex = CalculateCascadeIndex(wsPos);
			switch(cascadeIndex)
			{
				case 0 : outColor = outColor * vec4(0.8,0.2,0.2,1.0); break;
				case 1 : outColor = outColor * vec4(0.2,0.8,0.2,1.0); break;
				case 2 : outColor = outColor * vec4(0.2,0.2,0.8,1.0); break;
				case 3 : outColor = outColor * vec4(0.8,0.8,0.2,1.0); break;
			}
			break;
		}
	}
}