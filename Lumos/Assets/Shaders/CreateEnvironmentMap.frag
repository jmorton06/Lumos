#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "EnvironmentMapping.glslh"

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform sampler2D u_Texture;
// procedural mode has no panorama: the slot holds the clouds' noise instead
#define SKY_NOISE u_Texture
layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4 u_Parameters;   // w mode: 0 panorama, 1 procedural sky
	vec4 u_SunDirection; // xyz towards the sun
	vec4 u_SunColour;    // rgb illuminance (lux / 120000), w time
	vec4 u_HorizonTint;
	vec4 u_ZenithTint;
	vec4 u_CloudColour;
	vec4 u_CloudParams;
	vec4 u_CloudWindDir;
}
ubo;

#include "Sky.glslh"

layout(push_constant) uniform PushConsts
{
	uint cubeFaceIndex;
} pushConsts;

layout(location = 0) out vec4 outFrag;

#define MATH_PI 3.1415926535897932384626433832795


vec2 dirToUV(vec3 dir)
{
	return vec2(
		0.5f + 0.5f * atan(dir.z, dir.x) / MATH_PI,
		1.f - acos(dir.y) / MATH_PI);
}

vec3 uvToXYZ()
{
	vec2 uv = outTexCoord;
	uv = 2.0 * vec2(uv.x, uv.y) - vec2(1.0);
	
	int face = int(pushConsts.cubeFaceIndex);
	
	if(face == 0) 	   return vec3(     1.f,   uv.y,    -uv.x);
	else if(face == 1) return vec3(    -1.f,   uv.y,     uv.x);
	else if(face == 2) return vec3(   +uv.x,   -1.f,    +uv.y);
	else if(face == 3) return vec3(   +uv.x,    1.f,    -uv.y);
	else if(face == 4) return vec3(   +uv.x,   uv.y,      1.f);
	else			   return vec3(   -uv.x,  +uv.y,     -1.f); //if(face == 5) 
}

vec3 panoramaToCubeMap()
{
	vec3 cubeTC = GetCubeMapTexCoord(int(pushConsts.cubeFaceIndex), outTexCoord);

	//vec3 scan = uvToXYZ(); 
	vec3 direction = normalize(cubeTC);
	vec2 src = dirToUV(direction);

	return texture(u_Texture, src).rgb; //< get the color from the panorama
}

// What the scene is lit by: the same sky the skybox draws, without the sun's disc
// (the directional light is the sun).
vec3 ProceduralSky()
{
	vec3 dir = GetCubeMapTexCoord2(int(pushConsts.cubeFaceIndex), outTexCoord);

	SkyParams sky;
	sky.SunDir       = normalize(ubo.u_SunDirection.xyz);
	sky.SunE         = ubo.u_SunColour.rgb;
	sky.HorizonTint  = ubo.u_HorizonTint.rgb;
	sky.ZenithTint   = ubo.u_ZenithTint.rgb;
	sky.CloudColour  = ubo.u_CloudColour;
	sky.CloudParams  = ubo.u_CloudParams;
	sky.CloudWindDir = ubo.u_CloudWindDir;
	sky.Time         = ubo.u_SunColour.w;

	vec3 through;
	return SkyColour(dir, sky, 8, through);
}

void main()
{
	if(ubo.u_Parameters.w < 0.5)
		outFrag = vec4(clamp(panoramaToCubeMap(), 0.0, 50.0), 1.0);
	else
		outFrag = vec4(clamp(ProceduralSky(), 0.0, 50.0), 1.0);
}
