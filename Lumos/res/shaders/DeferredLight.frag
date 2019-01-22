#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(set = 1, binding = 0) uniform sampler2D uColourSampler;
layout(set = 1, binding = 1) uniform sampler2D uPositionSampler;
layout(set = 1, binding = 2) uniform sampler2D uNormalSampler;
layout(set = 1, binding = 3) uniform sampler2D uPBRSampler;
layout(set = 1, binding = 4) uniform sampler2D uPreintegratedFG;
layout(set = 1, binding = 5) uniform samplerCube uEnvironmentMap;
layout(set = 1, binding = 6) uniform sampler2DArrayShadow uShadowMap;
layout(set = 1, binding = 7) uniform sampler2D uDepthSampler;

layout(set = 0, binding = 0) uniform UniformBufferLight
{
	vec4 position;
 	vec4 direction;
 	vec4 cameraPosition;
	mat4 viewMatrix;
	mat4 uShadowTransform[16];
    vec4 uSplitDepths[16];
} ubo;

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2

struct Light
{
	vec3 position;
	vec3 colour;
	vec3 direction;
	float radius;
	float intensity;
	int type;
};

struct Material
{
	vec4 albedo;
	vec3 specular;
	float roughness;
	vec3 normal;
};

vec3 FinalGamma(vec3 color)
{
	return pow(color, vec3(1.0 / GAMMA));
}

vec3 GammaCorrectTextureRGB(vec3 texCol)
{
	return vec3(pow(texCol.rgb, vec3(GAMMA)));
}

float FresnelSchlick(float f0, float fd90, float view)
{
	return f0 + (fd90 - f0) * pow(max(1.0 - view, 0.1), 5.0);
}

float Disney(Light light, Material material, vec3 eye)
{
	vec3 halfVector = normalize(light.direction + eye);

	float NdotL = max(dot(material.normal, light.direction), 0.0);
	float LdotH = max(dot(light.direction, halfVector), 0.0);
	float NdotV = max(dot(material.normal, eye), 0.0);

	float energyBias = mix(0.0, 0.5, material.roughness);
	float energyFactor = mix(1.0, 1.0 / 1.51, material.roughness);
	float fd90 = energyBias + 2.0 * (LdotH * LdotH) * material.roughness;
	float f0 = 1.0;

	float lightScatter = FresnelSchlick(f0, fd90, NdotL);
	float viewScatter = FresnelSchlick(f0, fd90, NdotV);

	return lightScatter * viewScatter * energyFactor;
}

vec3 GGX(Light light, Material material, vec3 eye)
{
	vec3 h = normalize(light.direction + eye);
	float NdotH = max(dot(material.normal, h), 0.0);

	float rough2 = max(material.roughness * material.roughness, 2.0e-3); // capped so spec highlights doesn't disappear
	float rough4 = rough2 * rough2;

	float d = (NdotH * rough4 - NdotH) * NdotH + 1.0;
	float D = rough4 / (PI * (d * d));

	// Fresnel
	vec3 reflectivity = material.specular;
	float fresnel = 1.0;
	float NdotL = clamp(dot(material.normal, light.direction), 0.0, 1.0);
	float LdotH = clamp(dot(light.direction, h), 0.0, 1.0);
	float NdotV = clamp(dot(material.normal, eye), 0.0, 1.0);
	vec3 F = reflectivity + (fresnel - fresnel * reflectivity) * exp2((-5.55473 * LdotH - 6.98316) * LdotH);

	// geometric / visibility
	float k = rough2 * 0.5;
	float G_SmithL = NdotL * (1.0 - k) + k;
	float G_SmithV = NdotV * (1.0 - k) + k;
	float G = 0.25 / (G_SmithL * G_SmithV);

	return G * D * F;
}

vec3 RadianceIBLIntegration(float NdotV, float roughness, vec3 specular)
{
	vec2 preintegratedFG = texture(uPreintegratedFG, vec2(roughness, 1.0 - NdotV)).rg;
	return specular * preintegratedFG.r + preintegratedFG.g;
}

vec3 IBL(Light light, Material material, vec3 eye)
{
	float NdotV = max(dot(material.normal, eye), 0.0);

	vec3 reflectionVector = normalize(reflect(-eye, material.normal));
	float smoothness = 1.0 - material.roughness;
	float mipLevel = (1.0 - smoothness * smoothness) * 10.0;
	vec4 cs = textureLod(uEnvironmentMap, reflectionVector, mipLevel);
	vec3 result = pow(cs.xyz, vec3(GAMMA)) * RadianceIBLIntegration(NdotV, material.roughness, material.specular);

	vec3 diffuseDominantDirection = material.normal;
	float diffuseLowMip = 9.6;
	vec3 diffuseImageLighting = textureLod(uEnvironmentMap, diffuseDominantDirection, diffuseLowMip).rgb;
	diffuseImageLighting = pow(diffuseImageLighting, vec3(GAMMA));

	return result + diffuseImageLighting * material.albedo.rgb;
}

float Diffuse(Light light, Material material, vec3 eye)
{
	return Disney(light, material, eye);
}

vec3 Specular(Light light, Material material, vec3 eye)
{
	return GGX(light, material, eye);
}

float DoShadowTest(vec3 tsShadow, int tsLayer, vec2 pix)
{
	vec4 tCoord;
	tCoord.xyw = tsShadow;
	tCoord.z = float(tsLayer);

	if (tsLayer > 0)
	{
		return texture(uShadowMap, tCoord);
	}
	else
	{
		float shadow = 0.0f;
		for (float y = -1.5f; y <= 1.5f; y += 1.0f)
			for (float x = -1.5f; x <= 1.5f; x += 1.0f)
				shadow += texture(uShadowMap, tCoord + vec4(pix.x * x, pix.y * y, 0, 0));

		return shadow / 16.0f;
	}
}

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

#define ambient 0.3

float textureProj(vec4 P, vec2 offset, int cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.005;

	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
	{
		float dist = texture(uShadowMap, vec4(shadowCoord.st + offset, cascadeIndex, shadowCoord.z));
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias)
		{
			shadow = ambient;
		}
	}
	return shadow;

}

float filterPCF(vec4 sc, int cascadeIndex)
{
	ivec2 texDim = textureSize(uShadowMap, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}

layout(location = 0) out vec4 outColor;

const float NORMAL_BIAS = 0.002f;
const float RAW_BIAS 	= 0.00025f;
const int NUM_SHADOWMAPS = 4; //TODO : uniform

void main()
{
	vec4 colourTex   = texture(uColourSampler   , fragTexCoord);

    if(colourTex.w < 0.1)
        discard;

    vec3 positionTex = texture(uPositionSampler , fragTexCoord).rgb;
    vec4 pbrTex		 = texture(uPBRSampler   , fragTexCoord);
    vec3 normal      = normalize(texture(uNormalSampler, fragTexCoord).rgb);

    vec3  spec      = vec3(pbrTex.x);
	float roughness = pbrTex.y;

    vec3 wsPos      = positionTex;

    Light light;
    light.direction = ubo.direction.xyz;
    light.position  = ubo.position.xyz;
    light.colour    = vec3(1.0);
    light.intensity = 4.0;

    vec3 finalColour;

    Material material;
    material.albedo    = colourTex;
    material.specular  = spec;
    material.roughness = roughness;
    material.normal    = normal;

    vec3 eye      = normalize(ubo.cameraPosition.xyz - wsPos);
    vec4 diffuse  = vec4(0.0);
    vec3 specular = vec3(0.0);

	vec4 shadowWsPos = vec4(wsPos + normal * NORMAL_BIAS, 1.0f);
	float shadow = 0.0f;

	int cascadeIndex = 0;
	vec4 viewPos = ubo.viewMatrix * vec4(wsPos, 1.0);

	for(int i = 0; i < NUM_SHADOWMAPS - 1; ++i)
	{
		if(viewPos.z < ubo.uSplitDepths[i].x)
		{
			cascadeIndex = i + 1;
		}
	}

	int shadowMethod = 0;

	if(shadowMethod == 0)
	{
		//for (int layerIdx = 0; layerIdx < NUM_SHADOWMAPS; layerIdx++)
		{
			int layerIdx = cascadeIndex;
			vec4 hcsShadow = ubo.uShadowTransform[layerIdx] * shadowWsPos;

			if (abs(hcsShadow.x) <= 1.0f && abs(hcsShadow.y) <= 1.0f)
			{
				hcsShadow.z -= RAW_BIAS;
				hcsShadow.xyz = hcsShadow.xyz * 0.5f + 0.5f;

				shadow = DoShadowTest(hcsShadow.xyz, layerIdx, textureSize(uShadowMap, 0).xy);
				//break;
			}
		}
	}
	else
	{
		vec4 shadowCoord = (biasMat * ubo.uShadowTransform[cascadeIndex]) * vec4(wsPos, 1.0);

		const int enablePCF = 0;
		if (enablePCF == 1)
		{
			shadow = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
		}
		else
		{
			shadow = textureProj(shadowCoord, vec2(0.0), cascadeIndex);
		}
	}

    float NdotL = clamp(dot(material.normal, light.direction), 0.0, 1.0)  * shadow;
    diffuse  += NdotL * Diffuse(light, material, eye)  * light.intensity;
    specular += NdotL * Specular(light, material, eye) * light.intensity;

    diffuse = max(diffuse, vec4(0.1));

    //finalColour = material.albedo.xyz * diffuse.rgb + specular;
    finalColour = material.albedo.xyz * diffuse.rgb + (specular + IBL(light, material, eye));

	finalColour = FinalGamma(finalColour);
	outColor = vec4(finalColour, 1.0);
}
