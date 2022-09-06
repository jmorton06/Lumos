#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(set = 1, binding = 0) uniform sampler2D uColourSampler;
layout(set = 1, binding = 1) uniform sampler2D uPositionSampler;
layout(set = 1, binding = 2) uniform sampler2D uNormalSampler;
layout(set = 1, binding = 3) uniform sampler2D uPBRSampler;
layout(set = 1, binding = 4) uniform sampler2D uBRDFLUT;
layout(set = 1, binding = 5) uniform samplerCube uEnvMap;
layout(set = 1, binding = 6) uniform samplerCube uIrrMap;
layout(set = 1, binding = 7) uniform sampler2DArray uShadowMap;
layout(set = 1, binding = 8) uniform sampler2D uDepthSampler;

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2
#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 4

const int NumPCFSamples = 16;
const int numBlockerSearchSamples = 16;
const bool fadeCascades = false;
const float Epsilon = 0.00001;
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

struct Material
{
	vec4 Albedo;
	vec3 Metallic;
	float Roughness;
	vec3 Emissive;
	vec3 Normal;
	float AO;
	vec3 View;
	float NDotV;
};

layout(std140, binding = 0) uniform UBOLight
{
	Light lights[MAX_LIGHTS]; //64 bytes * 32
	mat4 ShadowTransform[MAX_SHADOWMAPS]; //64 * 4 = 256

	mat4 ViewMatrix; //64
	mat4 LightView; //64
	mat4 BiasMatrix; //64
	vec4 cameraPosition; //16
	vec4 SplitDepths[MAX_SHADOWMAPS]; //4 * 16 = 64
	float LightSize;
	float MaxShadowDist;
	float ShadowFade;
	float CascadeFade;
	int LightCount; //4
	int ShadowCount; // 4
	int Mode; // 4
	int EnvMipCount; // 4
	float InitialBias;
} ubo;

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

float Random(vec3 seed, int i)
{
	vec4 seed4 = vec4(seed, i);
	float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio   

float GoldNoise(vec2 xy, float seed)
{
	return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
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

float TextureProj(vec4 shadowCoord, vec2 offset, int cascadeIndex, float bias)
{
	float shadow = 1.0;
	float ambient = 0.0;
	
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 && shadowCoord.w > 0)
	{
		float dist = texture(uShadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (dist < (shadowCoord.z - bias))
		{
			shadow = ambient;//dist;
		}
	}
	return shadow;
	
}

float PCFShadow(vec4 sc, int cascadeIndex, float bias, vec3 wsPos)
{
	ivec2 texDim = textureSize(uShadowMap, 0).xy;
	float scale = 0.75;
	
	vec2 dx = scale * 1.0 / texDim;
	
	float shadowFactor = 0.0;
	int count = 0;
	float range = 1.0;
	
	for (float x = -range; x <= range; x += 1.0) 
	{
		for (float y = -range; y <= range; y += 1.0) 
		{
			shadowFactor += TextureProj(sc, vec2(dx.x * x, dx.y * y), cascadeIndex, bias);
			count++;
		}
	}
	return shadowFactor / count;
}

float PoissonShadow(vec4 sc, int cascadeIndex, float bias, vec3 wsPos)
{
	ivec2 texDim = textureSize(uShadowMap, 0).xy;
	float scale = 0.8;
	
	vec2 dx = scale * 1.0 / texDim;

	float shadowFactor = 1.0;
	int count = 0;

	for(int i = 0; i < 8; i ++)
	{
		int index = int(16.0*Random(floor(wsPos*1000.0), count))%16;
		shadowFactor -= 0.1 * (1.0 - TextureProj(sc, dx * PoissonDistribution16[index], cascadeIndex, bias));
		count++;
	}
	return shadowFactor;
}

vec2 SearchRegionRadiusUV(float zWorld)
{
	float light_zNear = 0.0; 
	vec2 lightRadiusUV = vec2(0.05);
    return lightRadiusUV * (zWorld - light_zNear) / zWorld;
}

float SearchWidth(float uvLightSize, float receiverDistance, vec3 cameraPos)
{
	const float NEAR = 0.1;
	return uvLightSize * (receiverDistance - NEAR) / cameraPos.z;
}

// PCF + Poisson + RandomSample model method
float PoissonDotShadow(vec4 sc, int cascadeIndex, float bias, vec3 wsPos)
{	
	float shadowMapDepth = 0.0;
	ivec2 texDim = textureSize(uShadowMap, 0).xy;
	
    for (int i = 0; i < NumPCFSamples; i++)
	{
		int index = int(float(NumPCFSamples)*GoldNoise(wsPos.xy, wsPos.z + i))%NumPCFSamples;
		vec2 pd = (2.0 / texDim) * PoissonDistribution[index];
		float z = texture(uShadowMap, vec3(sc.xy + SamplePoisson(index) * pd, cascadeIndex)).r;
		shadowMapDepth += (z < (sc.z - bias)) ? 1 : 0;
	}
	
	return shadowMapDepth / float(NumPCFSamples);
}

float GetShadowBias(vec3 lightDirection, vec3 normal)
{
	float MINIMUM_SHADOW_BIAS = ubo.InitialBias;
	float bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(normal, lightDirection)), MINIMUM_SHADOW_BIAS);
	return bias;
}

float FindBlockerDistance_DirectionalLight(sampler2DArray shadowMap, vec4 shadowCoords, float uvLightSize, vec3 lightDirection, vec3 normal, vec3 wsPos, int cascadeIndex)
{
	float bias = GetShadowBias(lightDirection, normal);

	int blockers = 0;
	float avgBlockerDistance = 0;
	
	float zEye = -(vec4(wsPos, 1.0) * ubo.LightView).z;
	vec2 searchWidth = SearchRegionRadiusUV(zEye);

	for (int i = 0; i < numBlockerSearchSamples; i++)
	{
		float z = texture(shadowMap, vec3(shadowCoords.xy + SamplePoisson(i) * searchWidth , cascadeIndex)).r;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
		return avgBlockerDistance / float(blockers);

	return -1;
}

float PCF_DirectionalLight(sampler2DArray shadowMap, vec4 shadowCoords, float uvRadius, vec3 lightDirection, vec3 normal, vec3 wsPos, int cascadeIndex)
{
	float bias = GetShadowBias(lightDirection, normal);
	float sum = 0;

	for (int i = 0; i < NumPCFSamples; i++)
	{
		int index = int(float(NumPCFSamples)*GoldNoise(wsPos.xy, wsPos.z * i))%NumPCFSamples;
		float z = texture(shadowMap, vec3(shadowCoords.xy + SamplePoisson(index)  * uvRadius, cascadeIndex)).r;
		sum += (z < (shadowCoords.z - bias)) ? 1 : 0;
	}
	return sum / NumPCFSamples;
}

float PCSS_DirectionalLight(sampler2DArray shadowMap, vec4 shadowCoords, float uvLightSize, vec3 lightDirection, vec3 normal, vec3 wsPos, int cascadeIndex)
{
	float blockerDistance = FindBlockerDistance_DirectionalLight(shadowMap, shadowCoords, uvLightSize, lightDirection, normal, wsPos, cascadeIndex);
	if (blockerDistance == -1)
		return 1;		

	float penumbraWidth = ( shadowCoords.z - blockerDistance) / blockerDistance;

	float NEAR = 0.01;
	float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z;

	return 1.0 - PCF_DirectionalLight(shadowMap, shadowCoords, uvRadius, lightDirection, normal, wsPos, cascadeIndex) * ShadowFade;
}

int CalculateCascadeIndex(vec3 wsPos)
{
	int cascadeIndex = 0;
	vec4 viewPos = vec4(wsPos, 1.0) * ubo.ViewMatrix;
	
	for(int i = 0; i < ubo.ShadowCount - 1; ++i)
	{
		if(viewPos.z < ubo.SplitDepths[i].x)
		{
			cascadeIndex = i + 1;
		}
	}
	
	return cascadeIndex;
}

float CalculateShadow(vec3 wsPos, int cascadeIndex, float bias, vec3 lightDirection, vec3 normal)
{
	vec4 shadowCoord =  vec4(wsPos, 1.0) * ubo.ShadowTransform[cascadeIndex] * ubo.BiasMatrix;

	if(cascadeIndex < 3)
	{	
		if (fadeCascades)
		{
			float CascadeFade = ubo.CascadeFade;
			vec4 viewPos = vec4(wsPos, 1.0) * ubo.ViewMatrix;
			float c0 = smoothstep(ubo.SplitDepths[0].x + CascadeFade * 0.5f, ubo.SplitDepths[0].x - CascadeFade * 0.5f, viewPos.z);
			float c1 = smoothstep(ubo.SplitDepths[1].x + CascadeFade * 0.5f, ubo.SplitDepths[1].x - CascadeFade * 0.5f, viewPos.z);
			float c2 = smoothstep(ubo.SplitDepths[2].x + CascadeFade * 0.5f, ubo.SplitDepths[2].x - CascadeFade * 0.5f, viewPos.z);
		
			if (c0 > 0.0 && c0 < 1.0)
			{
				// Sample 0 & 1
				vec4 shadowMapCoords = vec4(wsPos, 1.0) * ubo.ShadowTransform[0] * ubo.BiasMatrix;
				float shadowAmount0 = PCSS_DirectionalLight(uShadowMap, shadowMapCoords * ( 1.0 / shadowMapCoords.w), ubo.LightSize, lightDirection, normal, wsPos, 0 );
				shadowMapCoords = vec4(wsPos, 1.0) * ubo.ShadowTransform[1] * ubo.BiasMatrix;
				float shadowAmount1 = PCSS_DirectionalLight(uShadowMap, shadowMapCoords * ( 1.0 / shadowMapCoords.w), ubo.LightSize, lightDirection, normal, wsPos, 1);

				return mix(shadowAmount0, shadowAmount1, c0);
			}
			else if (c1 > 0.0 && c1 < 1.0)
			{
				// Sample 1 & 2
				vec4 shadowMapCoords = vec4(wsPos, 1.0) * ubo.ShadowTransform[1] * ubo.BiasMatrix;
				float shadowAmount0 = PCSS_DirectionalLight(uShadowMap, shadowMapCoords * ( 1.0 / shadowMapCoords.w), ubo.LightSize, lightDirection, normal, wsPos, 1 );
				shadowMapCoords = vec4(wsPos, 1.0) * ubo.ShadowTransform[2] * ubo.BiasMatrix;
				float shadowAmount1 = PCSS_DirectionalLight(uShadowMap, shadowMapCoords * ( 1.0 / shadowMapCoords.w), ubo.LightSize, lightDirection, normal, wsPos, 2);

				return mix(shadowAmount0, shadowAmount1, c0);
			}
			else if (c2 > 0.0 && c2 < 1.0)
			{
				// Sample 2 & 3
				vec4 shadowMapCoords = vec4(wsPos, 1.0) * ubo.ShadowTransform[2] * ubo.BiasMatrix;
				float shadowAmount0 = PCSS_DirectionalLight(uShadowMap, shadowMapCoords * ( 1.0 / shadowMapCoords.w), ubo.LightSize, lightDirection, normal, wsPos, 2 );
				shadowMapCoords = vec4(wsPos, 1.0) * ubo.ShadowTransform[3] * ubo.BiasMatrix;
				float bias = 0.0005;
				float shadowAmount1 = 1.0 - PoissonDotShadow(shadowMapCoords * ( 1.0 / shadowMapCoords.w), 3, bias, wsPos) * ShadowFade;
				return mix(shadowAmount0, shadowAmount1, c0);
			}
			else
			{
				return PCSS_DirectionalLight(uShadowMap, shadowCoord * ( 1.0 / shadowCoord.w), ubo.LightSize, lightDirection, normal, wsPos, cascadeIndex );
			}
		}
		else
		{
			return 1.0 - PoissonDotShadow(shadowCoord, cascadeIndex, bias, wsPos) * ShadowFade;
			//return PCSS_DirectionalLight(uShadowMap, shadowCoord * ( 1.0 / shadowCoord.w), ubo.LightSize, lightDirection, normal, wsPos, cascadeIndex );
		}
	}
	else
	{
		float bias = 0.0005;
		return 1.0 - PoissonDotShadow(shadowCoord, cascadeIndex, bias, wsPos) * ShadowFade;
	}
}

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;
	
	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

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

			value = atten;

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
			float bias = ubo.InitialBias;
			bias = bias + (bias * tan(acos(clamp(dot(material.Normal, light.direction.xyz), 0.0, 1.0))) * 0.5);
			
			int cascadeIndex = CalculateCascadeIndex(wsPos);
			value = CalculateShadow(wsPos,cascadeIndex, bias, light.direction.xyz, material.Normal);
		}

		vec3 Li = light.direction.xyz;
		vec3 Lradiance = light.colour.xyz * light.intensity;
		vec3 Lh = normalize(Li + material.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(material.Normal, Li));
		float cosLh = max(0.0, dot(material.Normal, Lh));

		//vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, material.View)));
		vec3 F = fresnelSchlickRoughness(F0, max(0.0, dot(Lh,  material.View)), material.Roughness);
		
		float D = ndfGGX(cosLh, material.Roughness);
		float G = gaSchlickGGX(cosLi, material.NDotV, material.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - material.Metallic.x);
		vec3 diffuseBRDF = kd * material.Albedo.xyz;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * material.NDotV);

		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi * value * material.AO;
	}
	return result;
}

vec3 RadianceIBLIntegration(float NdotV, float roughness, vec3 metallic)
{
	vec2 preintegratedFG = texture(uBRDFLUT, vec2(roughness, 1.0 - NdotV)).rg;
	return metallic * preintegratedFG.r + preintegratedFG.g;
}

vec3 IBL(vec3 F0, vec3 Lr, Material material)
{
	vec3 irradiance = texture(uIrrMap, material.Normal).rgb;
	vec3 F = fresnelSchlickRoughness(F0, material.NDotV, material.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - material.Metallic.x);
	vec3 diffuseIBL = material.Albedo.xyz * irradiance;

	int u_EnvRadianceTexLevels = ubo.EnvMipCount;// textureQueryLevels(uBRDFLUT);	
	vec3 specularIrradiance = textureLod(uEnvMap, Lr, material.Roughness * u_EnvRadianceTexLevels).rgb;

	vec2 specularBRDF = texture(uBRDFLUT, vec2(material.NDotV, 1.0 - material.Roughness.x)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

vec3 FinalGamma(vec3 colour)
{
	return pow(colour, vec3(1.0 / GAMMA));
}

vec3 GammaCorrectTextureRGB(vec3 texCol)
{
	return vec3(pow(texCol.rgb, vec3(GAMMA)));
}

float Attentuate( vec3 lightData, float dist )
{
	float att =  1.0 / ( lightData.x + lightData.y*dist + lightData.z*dist*dist );
	float damping = 1.0;// - (dist/lightData.w);
	return max(att * damping, 0.0);
}

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 colourTex   = texture(uColourSampler   , fragTexCoord);
	
	if(colourTex.w < 0.1)
        discard;

    vec4 positionTex = texture(uPositionSampler , fragTexCoord);
    vec4 pbrTex		 = texture(uPBRSampler      , fragTexCoord);
    vec4 normalTex   = texture(uNormalSampler   , fragTexCoord);

    vec3  spec      = vec3(pbrTex.x);

	float roughness = pbrTex.y;
	vec3 emissive	= vec3(positionTex.w, normalTex.w, pbrTex.w);
    vec3 wsPos      = positionTex.xyz;
	vec3 normal		= normalize(normalTex.xyz);

	Material material;
    material.Albedo    = colourTex;
    material.Metallic  = spec;
    material.Roughness = max(roughness, 0.05);
    material.Normal    = normal;
	material.AO		   = pbrTex.z;
	material.Emissive  = emissive;
	material.View 	   = normalize(ubo.cameraPosition.xyz - wsPos);
	material.NDotV     = max(dot(material.Normal, material.View), 0.0);

	float shadowDistance = ubo.MaxShadowDist;
	float transitionDistance = ubo.ShadowFade;

	vec4 viewPos = vec4(wsPos, 1.0) * ubo.ViewMatrix;

	float distance = length(viewPos);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0);
	
	vec3 Lr = 2.0 * material.NDotV * material.Normal - material.View;
	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, material.Albedo.xyz, material.Metallic.x);

	vec3 lightContribution = Lighting(F0, wsPos, material);
	vec3 iblContribution = IBL(F0, Lr, material) * 2.0;

	vec3 finalColour = lightContribution + iblContribution + emissive;
	outColor = vec4(finalColour, 1.0);

	if(ubo.Mode > 0)
	{
		switch(ubo.Mode)
		{
			case 1:
				outColor = colourTex;
				break;
			case 2:
				outColor = vec4(material.Metallic, 1.0);
				break;
			case 3:
				outColor = vec4(material.Roughness, material.Roughness, material.Roughness,1.0);
				break;
			case 4:
				outColor = vec4(material.AO, material.AO, material.AO, 1.0);
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


