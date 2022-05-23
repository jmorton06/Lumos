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


void main(void)
{
	const float alphaCutOut = 0.1;
	float alpha = texture(u_AlbedoMap, fragTexCoord).a;
	
	if(alpha < alphaCutOut)
		discard;
}