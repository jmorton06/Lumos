#shader vertex

uniform mat4 sys_projView;
uniform mat4 modelMatrix;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;


out vec3 fragColor;
out vec2 fragTexCoord;
out vec4 fragPosition;
out vec3 fragNormal;
out vec3 fragTangent;

void main() 
{
	fragPosition = vec4(inPosition, 1.0) * modelMatrix;
	gl_Position = fragPosition * sys_projView;
	
	fragColor = inColor;
	fragTexCoord = inTexCoord;
	fragNormal = normalize(inNormal) * transpose(inverse(mat3(modelMatrix)));
	fragTangent = inTangent;
}
#shader end

#shader fragment

in vec3 fragColor;
in vec2 fragTexCoord;
in vec4 fragPosition;
in vec3 fragNormal;
in vec3 fragTangent;

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_SpecularMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_EmissiveMap;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outPBR;
layout(location = 4) out vec4 outDepth;

layout (std140) uniform UniformMaterialData
{
	vec4  albedoColour;
	vec4  RoughnessColour;
	vec4  specularColour;
	float usingAlbedoMap;
	float usingSpecularMap;
	float usingRoughnessMap;
	float usingNormalMap;
	float usingAOMap;
	float usingEmissiveMap;
	int workflow;
	float p1;
} materialProperties;

const int PBR_WORKFLOW_SEPARATE_TEXTURES = 0;
const int PBR_WORKFLOW_METALLIC_ROUGHNESS = 1;
const int PBR_WORKFLOW_SPECULAR_GLOSINESS = 2;

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
	return (1.0 - materialProperties.usingSpecularMap) * materialProperties.specularColour.rgb + materialProperties.usingSpecularMap * texture(u_SpecularMap, fragTexCoord).rgb;
}

float GetRoughness()
{
	return (1.0 - materialProperties.usingRoughnessMap) * materialProperties.RoughnessColour.r + materialProperties.usingRoughnessMap * texture(u_RoughnessMap, fragTexCoord).r;
}

float GetAO()
{
	return (1.0 - materialProperties.usingAOMap) +  materialProperties.usingAOMap * GammaCorrectTextureRGB(texture(u_AOMap, fragTexCoord)).r;
}

vec3 GetEmissive()
{
	return materialProperties.usingEmissiveMap * GammaCorrectTextureRGB(texture(u_EmissiveMap, fragTexCoord));
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

	float specular = 0.0;
	float roughness = 0.0;

	if(materialProperties.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		specular  = GetSpecular().x;
		roughness = GetRoughness();
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex = GammaCorrectTextureRGB(texture(u_SpecularMap, fragTexCoord));
		specular = tex.b;
		roughness = tex.g;
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		vec3 tex = GammaCorrectTextureRGB(texture(u_SpecularMap, fragTexCoord));
		specular = tex.b;
		roughness = tex.g;
	}
	
	vec3 emissive   = GetEmissive();
	float ao		= GetAO();

	outColor    = texColour;
	outPosition = fragPosition;
	outNormal   = vec4(GetNormalFromMap(),1.0);
	outPBR      = vec4(specular,roughness, ao, 1.0);

	outPosition.w = emissive.x;
	outNormal.w   = emissive.y;
	outPBR.w      = emissive.z;
}
#shader end
