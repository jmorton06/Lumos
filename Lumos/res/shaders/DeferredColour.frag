#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;


layout(set = 1, binding = 0) uniform sampler2D u_AlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D u_SpecularMap;
layout(set = 1, binding = 2) uniform sampler2D u_GlossMap;
layout(set = 1, binding = 3) uniform sampler2D u_NormalMap;

layout(set = 1,binding = 4) uniform UniformMaterialData
{
	float usingAlbedoMap;
	float usingSpecularMap;
	float usingGlossMap;
	float usingNormalMap;
	vec4  albedoColour;
	vec4  glossColour;
	vec4  specularColour;
} materialProperties;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outPBR;
layout(location = 4) out vec4 outDepth;

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2

vec4 GammaCorrectTexture(vec4 samp)
{
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

vec3 GammaCorrectTextureRGB(vec4 samp)
{
	return vec3(pow(samp.rgb, vec3(GAMMA)));
}

vec4 GetAlbedo()
{
	return (1.0 - materialProperties.usingAlbedoMap) * materialProperties.albedoColour + materialProperties.usingAlbedoMap * GammaCorrectTexture(texture(u_AlbedoMap, fragTexCoord));
}

vec3 GetSpecular()
{
	return (1.0 - materialProperties.usingSpecularMap) * materialProperties.specularColour.xyz + materialProperties.usingSpecularMap * GammaCorrectTextureRGB(texture(u_SpecularMap, fragTexCoord));
}

float GetGloss()
{
	return (1.0 - materialProperties.usingGlossMap) *  materialProperties.glossColour.r +  materialProperties.usingGlossMap * GammaCorrectTextureRGB(texture(u_GlossMap, fragTexCoord)).r;
}

float GetRoughness()
{
	return 1.0 - GetGloss();
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

	vec3 specular   = GetSpecular();
	float roughness = GetGloss();

    outColor    = texColour;
	outPosition = fragPosition;

	outNormal   = vec4(GetNormalFromMap(),1.0);
	outPBR      = vec4(specular.x,roughness, 1.0,1.0);
}