#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;

layout(set = 1, binding = 0) uniform sampler2D u_AlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D u_MetallicMap;
layout(set = 1, binding = 2) uniform sampler2D u_RoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D u_NormalMap;
layout(set = 1, binding = 4) uniform sampler2D u_AOMap;
layout(set = 1, binding = 5) uniform sampler2D u_EmissiveMap;

layout(set = 1,binding = 6) uniform UniformMaterialData
{
	vec4  albedoColour;
	vec4  RoughnessColour;
	vec4  metallicColour;
	vec4  emissiveColour;
	float usingAlbedoMap;
	float usingMetallicMap;
	float usingRoughnessMap;
	float usingNormalMap;
	float usingAOMap;
	float usingEmissiveMap;
	float workflow;
	float padding;
} materialProperties;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outPBR;
layout(location = 4) out vec4 outDepth;

const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2

vec4 GammaCorrectTexture(vec4 samp)
{
	return samp;
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

vec3 GammaCorrectTextureRGB(vec4 samp)
{
	return samp.xyz;
	return vec3(pow(samp.rgb, vec3(GAMMA)));
}

vec4 GetAlbedo()
{
	return (1.0 - materialProperties.usingAlbedoMap) * materialProperties.albedoColour + materialProperties.usingAlbedoMap * GammaCorrectTexture(texture(u_AlbedoMap, fragTexCoord));
}

vec3 GetMetallic()
{
	return (1.0 - materialProperties.usingMetallicMap) * materialProperties.metallicColour.rgb + materialProperties.usingMetallicMap * GammaCorrectTextureRGB(texture(u_MetallicMap, fragTexCoord)).rgb;
}

float GetRoughness()
{
	return (1.0 - materialProperties.usingRoughnessMap) *  materialProperties.RoughnessColour.r + materialProperties.usingRoughnessMap * GammaCorrectTextureRGB(texture(u_RoughnessMap, fragTexCoord)).r;
}

float GetAO()
{
	return materialProperties.usingAOMap * GammaCorrectTextureRGB(texture(u_AOMap, fragTexCoord)).r;
}

vec3 GetEmissive()
{
	return (1.0 - materialProperties.usingEmissiveMap) * materialProperties.emissiveColour.rgb + materialProperties.usingEmissiveMap * GammaCorrectTextureRGB(texture(u_EmissiveMap, fragTexCoord));
}

vec3 GetNormalFromMap()
{
	if (materialProperties.usingNormalMap < 0.1)
		return normalize(fragNormal);

	vec3 tangentNormal = texture(u_NormalMap, fragTexCoord).xyz * 2.0 - 1.0;

	vec3 Q1 = dFdx(fragPosition.xyz);
	vec3 Q2 = dFdy(fragPosition.xyz);
	vec2 st1 = dFdx(fragTexCoord);
	vec2 st2 = dFdy(fragTexCoord);

	vec3 N = normalize(fragNormal);
	vec3 T = normalize(Q1*st2.t - Q2*st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main()
{
	vec4 texColour = GetAlbedo();
	if(texColour.w < 0.4)
		discard;

	float metallic = 0.0;
	float roughness = 0.0;

	if(materialProperties.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		metallic  = GetMetallic().x;
		roughness = GetRoughness();
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex = GammaCorrectTextureRGB(texture(u_MetallicMap, fragTexCoord));
		metallic = tex.b;
		roughness = tex.g;
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		vec3 tex = GammaCorrectTextureRGB(texture(u_MetallicMap, fragTexCoord));
		metallic = tex.b;
		roughness = tex.g;
	}

	vec3 emissive   = GetEmissive();
	float ao		= GetAO();

    outColor    = texColour;
	outPosition = fragPosition;
	outNormal   = vec4(GetNormalFromMap(),1.0);
	outPBR      = vec4(metallic,roughness, ao, 1.0);

	outPosition.w = emissive.x;
	outNormal.w   = emissive.y;
	outPBR.w      = emissive.z;
}