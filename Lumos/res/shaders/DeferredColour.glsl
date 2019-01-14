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
out vec4 position;
out vec3 normal;
out vec3 tangent;

void main() 
{
    gl_Position 	= sys_projView * modelMatrix * vec4(inPosition, 1.0);
	position 		= modelMatrix * vec4(inPosition, 1.0);
    fragColor 		= inColor;
	fragTexCoord 	= inTexCoord;
	normal 			= transpose(inverse(mat3(modelMatrix))) * normalize(inNormal);
}
#shader end

#shader fragment

in vec3 fragColor;
in vec2 fragTexCoord;
in vec4 position;
in vec3 normal;
in vec3 tangent;

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_SpecularMap;
uniform sampler2D u_GlossMap;
uniform sampler2D u_NormalMap;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outPBR;
layout(location = 4) out vec4 outDepth;

layout (std140) uniform UniformMaterialData
{
	float glossColour;
	float usingAlbedoMap;
	float usingSpecularMap;
	float usingGlossMap;
	float usingNormalMap;
	vec4  albedoColour;
	vec4  specularColour;
};

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2

vec4 GammaCorrectTexture(sampler2D tex, vec2 uv)
{
	vec4 samp = texture(tex, uv);
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

vec3 GammaCorrectTextureRGB(sampler2D tex, vec2 uv)
{
	vec4 samp = texture(tex, uv);
	return vec3(pow(samp.rgb, vec3(GAMMA)));
}

vec4 GetAlbedo()
{
	return (1.0 - usingAlbedoMap) * albedoColour + usingAlbedoMap * GammaCorrectTexture(u_AlbedoMap, fragTexCoord);
}

vec3 GetSpecular()
{
	return (1.0 - usingSpecularMap) * specularColour.xyz + usingSpecularMap * texture(u_SpecularMap, fragTexCoord).rgb;//GammaCorrectTextureRGB(u_SpecularMap, fragTexCoord);
}

float GetGloss()
{
	return (1.0 - usingGlossMap) *  glossColour +  usingGlossMap * texture(u_GlossMap, fragTexCoord).r;//GammaCorrectTextureRGB(u_GlossMap, fragTexCoord).r;
}

float GetRoughness()
{
	return 1.0 - GetGloss();
}

vec3 GetNormalFromMap()
{
	if (usingNormalMap < 0.1)
		return normalize(normal);

	vec3 tangentNormal = texture(u_NormalMap, fragTexCoord).xyz * 2.0 - 1.0;

	vec3 Q1 = dFdx(position.xyz);
	vec3 Q2 = dFdy(position.xyz);
	vec2 st1 = dFdx(fragTexCoord);
	vec2 st2 = dFdy(fragTexCoord);

	vec3 N = normalize(normal);
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

	vec3 specular   		= GetSpecular();
	float roughness 		= GetGloss();

    outColor    = texColour;
	outPosition = position;

	outNormal   = vec4(GetNormalFromMap(),1.0);
	outPBR      = vec4(specular.x,roughness, 1.0,1.0);
	outDepth 	= vec4(position.z,0.0,0.0,1.0);
}
#shader end