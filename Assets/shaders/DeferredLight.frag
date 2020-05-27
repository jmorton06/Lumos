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
layout(set = 1, binding = 6) uniform samplerCube uIrradianceMap;
layout(set = 1, binding = 7) uniform sampler2DArray uShadowMap;
layout(set = 1, binding = 8) uniform sampler2D uDepthSampler;

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 16

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

layout(std140, binding = 0) uniform UniformBufferLight
{
	Light lights[MAX_LIGHTS];
 	vec4 cameraPosition;
	mat4 viewMatrix;
	mat4 uShadowTransform[MAX_SHADOWMAPS];
    vec4 uSplitDepths[MAX_SHADOWMAPS];
	int lightCount;
	int shadowCount;
	int mode;
	int cubemapMipLevels;
} ubo;

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2

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

const float Epsilon = 0.00001;

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

vec3 Lighting(vec3 F0, float shadow, vec3 wsPos, Material material)
{
	vec3 result = vec3(0.0);

	for(int i = 0; i < ubo.lightCount; i++)
	{
		Light light = ubo.lights[i];

		float value = shadow;

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
			float intensity 	= attenuation * attenuation;
			
			// Erase light if there is no need to compute it
			intensity *= step(theta, cutoffAngle);

			value = intensity;
		}

		vec3 Li = light.direction.xyz;
		vec3 Lradiance = light.colour.xyz * light.intensity;
		vec3 Lh = normalize(Li + material.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(material.Normal, Li));
		float cosLh = max(0.0, dot(material.Normal, Lh));

		vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, material.View)));
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
	vec2 preintegratedFG = texture(uPreintegratedFG, vec2(roughness, 1.0 - NdotV)).rg;
	return metallic * preintegratedFG.r + preintegratedFG.g;
}

vec3 IBL(vec3 F0, vec3 Lr, Material material)
{
	vec3 irradiance = texture(uIrradianceMap, material.Normal).rgb;
	vec3 F = fresnelSchlickRoughness(F0, material.NDotV, material.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - material.Metallic.x);
	vec3 diffuseIBL = material.Albedo.xyz * irradiance;

	int u_EnvRadianceTexLevels = ubo.cubemapMipLevels;// textureQueryLevels(uPreintegratedFG);	
	vec3 specularIrradiance = textureLod(uEnvironmentMap, Lr, material.Roughness * u_EnvRadianceTexLevels).rgb;

	vec2 specularBRDF = texture(uPreintegratedFG, vec2(material.NDotV, 1.0 - material.Roughness.x)).rg;
	vec3 specularIBL = specularIrradiance * (F * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

vec3 FinalGamma(vec3 color)
{
	return pow(color, vec3(1.0 / GAMMA));
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

float textureProj(vec4 P, vec2 offset, int cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.001; //0.005

	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
	{
		float dist = texture(uShadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).x;//, shadowCoord.z));
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias)
		{
			shadow = 0.1f;
		}
	}
	return shadow;

}

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.5,
	0.0, 0.5, 0.0, 0.5,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
);

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

int CalculateCascadeIndex(vec3 wsPos)
{
	int cascadeIndex = 0;
	vec4 viewPos = vec4(wsPos, 1.0) * ubo.viewMatrix;

	for(int i = 0; i < ubo.shadowCount - 1; ++i)
	{
		if(viewPos.z < ubo.uSplitDepths[i].x)
		{
			cascadeIndex = i + 1;
		}
	}

	return cascadeIndex;
}

float CalculateShadow(vec3 wsPos, int cascadeIndex)
{
	vec4 shadowCoord =  vec4(wsPos, 1.0) * ubo.uShadowTransform[cascadeIndex] * biasMat;
	return filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
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
	vec3 emissive	= vec3(positionTex.w, normalTex.w, pbrTex.w);// EncodeFloatRGBA(pbrTex.w).xyz;
    vec3 wsPos      = positionTex.xyz;
	vec3 normal		= normalize(normalTex.xyz);
    vec3 finalColour;

	Material material;
    material.Albedo    = colourTex;
    material.Metallic  = spec;
    material.Roughness = max(roughness, 0.05);
    material.Normal    = normal;
	material.AO		   = pbrTex.z;
	material.Emissive  = emissive;
	material.View 	   = normalize(ubo.cameraPosition.xyz - wsPos);
	material.NDotV 	   = max(dot(material.Normal, material.View), 0.0);

	int cascadeIndex = CalculateCascadeIndex(wsPos);
	float shadow = CalculateShadow(wsPos,cascadeIndex);

	vec3 Lr = 2.0 * material.NDotV * material.Normal - material.View;
	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, material.Albedo.xyz, material.Metallic.x);

	vec3 lightContribution = Lighting(F0, shadow, wsPos, material);
	vec3 iblContribution = IBL(F0, Lr, material) * 2.0;

	finalColour = lightContribution + iblContribution + emissive;
	outColor = vec4(finalColour, 1.0);

	if(ubo.mode > 0)
	{
		switch(ubo.mode)
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
