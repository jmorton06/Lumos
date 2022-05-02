#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(push_constant) uniform PushConsts
{
	uint cubeFaceIndex;
} pushConsts;

layout(set = 0, binding = 0) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;

#define MATH_PI 3.1415926535897932384626433832795


vec3 uvToXYZ(int face, vec2 uv)
{
	if(face == 0)
		return vec3(     1.f,   uv.y,    -uv.x);

	else if(face == 1)
		return vec3(    -1.f,   uv.y,     uv.x);

	else if(face == 2)
		return vec3(   +uv.x,   -1.f,    +uv.y);

	else if(face == 3)
		return vec3(   +uv.x,    1.f,    -uv.y);

	else if(face == 4)
		return vec3(   +uv.x,   uv.y,      1.f);

	else //if(face == 5)
	{	return vec3(    -uv.x,  +uv.y,     -1.f);}
}

vec2 dirToUV(vec3 dir)
{
	return vec2(
		0.5f + 0.5f * atan(dir.z, dir.x) / MATH_PI,
		1.f - acos(dir.y) / MATH_PI);
}

vec3 panoramaToCubeMap(int face, vec2 texCoord)
{
	vec2 texCoordNew = texCoord*2.0-1.0; //< mapping vom 0,1 to -1,1 coords
	vec3 scan = uvToXYZ(face, texCoordNew); 
	vec3 direction = normalize(scan);
	vec2 src = dirToUV(direction);

	return texture(u_Texture, src).rgb; //< get the color from the panorama
}

const float coeiff = 0.3;
const vec3 totalSkyLight = vec3(0.3, 0.5, 1.0);

const float PI = 3.14159265359;
const float blurLevel = 1.0f;
const float timeCounter = 0.0f;

vec3 mie(float dist, vec3 sunL)
{
	return max(exp(-pow(dist, 0.25)) * sunL - 0.4, 0.0);
}

vec3 GetCubeMapTexCoord()
{
	vec2 uv = outTexCoord;
	uv = 2.0 * vec2(uv.x, 1.0 - uv.y) - vec2(1.0);
	
    vec3 ret;
	int face = int(pushConsts.cubeFaceIndex);
	vec3 GlobalInvocationID = vec3(0.0,0.0,face);
    if (GlobalInvocationID.z == 0)      ret = vec3(  1.0, uv.y, -uv.x);
    else if (GlobalInvocationID.z == 1) ret = vec3( -1.0, uv.y,  uv.x);
    else if (GlobalInvocationID.z == 2) ret = vec3( uv.x,  1.0, -uv.y);
    else if (GlobalInvocationID.z == 3) ret = vec3( uv.x, -1.0,  uv.y);
    else if (GlobalInvocationID.z == 4) ret = vec3( uv.x, uv.y,   1.0);
    else if (GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y,  -1.0);
    return normalize(ret);
}

vec3 getSky()
{
	int face = int(pushConsts.cubeFaceIndex);
	vec2 texCoordNew = outTexCoord*2.0-1.0; //< mapping vom 0,1 to -1,1 coords
	vec3 scan = uvToXYZ(face, texCoordNew); 
	vec3 direction = normalize(scan);
	vec2 src = dirToUV(direction);
	vec3 uv = GetCubeMapTexCoord();//vec3(src, face);
	//vec3 uv = normalize(outPosition.xyz);
	
	vec3 sunPos;
	float radian = PI * 2.0 *((timeCounter / 86400.0) - 0.333333);
	sunPos.x = cos(radian);
	sunPos.y = sin(radian);
	sunPos.z = 0.0;
	
	float sunDistance = length(uv - normalize(sunPos));// clamp(sunPos, -1.0, 1.0));
	
	float scatterMult = clamp(sunDistance, 0.0, 1.0);
	//float sun = clamp(1.0 - smoothstep(0.01, 0.011, scatterMult), 0.0, 1.0);
	
	float dist = uv.y + 0.1; // reduce horizon level
	dist = (coeiff * mix(scatterMult, 1.0, dist)) / dist;
	
	//vec3 mieScatter = mie(sunDistance, vec3(1.0));
	
	vec3 colour = dist * totalSkyLight;
	
	colour = max(colour, 0.0);
	colour = max(mix(pow(colour, 1.0 - colour),
					 colour / (2.0 * colour + 0.5 - colour),
					 clamp(sunPos.y * 2.0, 0.0, 1.0)), 0.0)
		;// +mieScatter;// +sun;
	
	//	colour *= 1.0 + pow(1.0 - scatterMult, 10.0) * 10.0;
	
	//float underscatter = (sunPos.y * 0.5 + 0.5 - 0.8);
	
	//vec3 nightcolour = texture(cubeTex, uv).xyz;
	
	//colour = mix(colour, nightcolour, clamp(underscatter, 0.0, 1.0));
	
	return colour;
}

void main()
{
	//vec3 colour = panoramaToCubeMap(int(pushConsts.cubeFaceIndex), outTexCoord);
	vec3 colour = getSky();
	outFrag = vec4(colour, 1.0);
}