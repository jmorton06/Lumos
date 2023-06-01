#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "EnvironmentMapping.glslh"

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform sampler2D u_Texture;
layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4 u_Parameters;
}
ubo;

layout(push_constant) uniform PushConsts
{
	uint cubeFaceIndex;
} pushConsts;

layout(location = 0) out vec4 outFrag;

const float coeiff = 0.3;
const vec3 totalSkyLight = vec3(0.3, 0.5, 1.0);

const float blurLevel = 1.0f;
const float timeCounter = 0.0f;

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

vec3 getSky()
{
	vec3 uv = GetCubeMapTexCoord2(int(pushConsts.cubeFaceIndex), outTexCoord);
	vec3 sunPos;
	float radian = MATH_PI * 2.0 *((timeCounter / 86400.0) - 0.333333);
	sunPos.x = cos(radian);
	sunPos.y = sin(radian);
	sunPos.z = 0.0;
	
	float sunDistance = length(uv - normalize(sunPos));// clamp(sunPos, -1.0, 1.0));
	
	float scatterMult = clamp(sunDistance, 0.0, 1.0);
	
	float dist = uv.y + 0.1; // reduce horizon level
	dist = (coeiff * mix(scatterMult, 1.0, dist)) / dist;
	
	vec3 colour = dist * totalSkyLight;
	
	colour = max(colour, 0.0);
	colour = max(mix(pow(colour, 1.0 - colour),
					 colour / (2.0 * colour + 0.5 - colour),
					 clamp(sunPos.y * 2.0, 0.0, 1.0)), 0.0);
	
	return colour;
}

vec3 mie(float dist, vec3 sunL)
{
	return max(exp(-pow(dist, 0.25)) * sunL - 0.4, 0.0);
}

vec3 YxyToXYZ( in vec3 Yxy )
{
	float Y = Yxy.r;
	float x = Yxy.g;
	float y = Yxy.b;

	float X = x * ( Y / y );

	float Z = ( 1.0 - x - y ) * ( Y / y );

	return vec3(X,Y,Z);
}

vec3 XYZToRGB( in vec3 XYZ )
{
	// CIE/E
	mat3 M = mat3
	(
		 2.3706743, -0.9000405, -0.4706338,
		-0.5138850,  1.4253036,  0.0885814,
 		 0.0052982, -0.0146949,  1.0093968
	);

	return XYZ * M;
}


vec3 YxyToRGB( in vec3 Yxy )
{
	vec3 XYZ = YxyToXYZ( Yxy );
	vec3 RGB = XYZToRGB( XYZ );
	return RGB;
}

float saturatedDot( in vec3 a, in vec3 b )
{
	return max( dot( a, b ), 0.01 );   
}

void calculatePerezDistribution( in float t, out vec3 A, out vec3 B, out vec3 C, out vec3 D, out vec3 E )
{
	A = vec3(  0.1787 * t - 1.4630, -0.0193 * t - 0.2592, -0.0167 * t - 0.2608 );
	B = vec3( -0.3554 * t + 0.4275, -0.0665 * t + 0.0008, -0.0950 * t + 0.0092 );
	C = vec3( -0.0227 * t + 5.3251, -0.0004 * t + 0.2125, -0.0079 * t + 0.2102 );
	D = vec3(  0.1206 * t - 2.5771, -0.0641 * t - 0.8989, -0.0441 * t - 1.6537 );
	E = vec3( -0.0670 * t + 0.3703, -0.0033 * t + 0.0452, -0.0109 * t + 0.0529 );
}

vec3 calculateZenithLuminanceYxy( in float t, in float thetaS )
{
	float chi  	 	= ( 4.0 / 9.0 - t / 120.0 ) * ( MATH_PI - 2.0 * thetaS );
	float Yz   	 	= ( 4.0453 * t - 4.9710 ) * tan( chi ) - 0.2155 * t + 2.4192;

	float theta2 	= thetaS * thetaS;
    float theta3 	= theta2 * thetaS;
    float T 	 	= t;
    float T2 	 	= t * t;

	float xz =
      ( 0.00165 * theta3 - 0.00375 * theta2 + 0.00209 * thetaS + 0.0)     * T2 +
      (-0.02903 * theta3 + 0.06377 * theta2 - 0.03202 * thetaS + 0.00394) * T +
      ( 0.11693 * theta3 - 0.21196 * theta2 + 0.06052 * thetaS + 0.25886);

    float yz =
      ( 0.00275 * theta3 - 0.00610 * theta2 + 0.00317 * thetaS + 0.0)     * T2 +
      (-0.04214 * theta3 + 0.08970 * theta2 - 0.04153 * thetaS + 0.00516) * T +
      ( 0.15346 * theta3 - 0.26756 * theta2 + 0.06670 * thetaS + 0.26688);

	return vec3( Yz, xz, yz );
}

vec3 calculatePerezLuminanceYxy( in float theta, in float gamma, in vec3 A, in vec3 B, in vec3 C, in vec3 D, in vec3 E )
{
	return ( 1.0 + A * exp( B / cos( theta ) ) ) * ( 1.0 + C * exp( D * gamma ) + E * cos( gamma ) * cos( gamma ) );
}

vec3 calculateSkyLuminanceRGB( in vec3 s, in vec3 e, in float t )
{
	vec3 A, B, C, D, E;
	calculatePerezDistribution( t, A, B, C, D, E );

	float thetaS = acos( saturatedDot( s, vec3(0,1,0) ) );
	float thetaE = acos( saturatedDot( e, vec3(0,1,0) ) );
	float gammaE = acos( saturatedDot( s, e )		   );

	vec3 Yz = calculateZenithLuminanceYxy( t, thetaS );

	vec3 fThetaGamma = calculatePerezLuminanceYxy( thetaE, gammaE, A, B, C, D, E );
	vec3 fZeroThetaS = calculatePerezLuminanceYxy( 0.0,    thetaS, A, B, C, D, E );
	vec3 Yp = Yz * ( fThetaGamma / fZeroThetaS );

	return YxyToRGB( Yp );
}

vec3 PreethamSky()
{
	vec3 cubeTexCoords = normalize(GetCubeMapTexCoord(int(pushConsts.cubeFaceIndex), outTexCoord));//uvToXYZ());

	float turbidity     = ubo.u_Parameters.x;
    float azimuth       = ubo.u_Parameters.y;
    float inclination   = ubo.u_Parameters.z;
    vec3 sunDir     	= normalize( vec3( sin(inclination) * cos(azimuth), cos(inclination), sin(inclination) * sin(azimuth) ) );
    vec3 viewDir  		= -cubeTexCoords;
    vec3 skyLuminance 	= calculateSkyLuminanceRGB( sunDir, viewDir, turbidity );
    
	return skyLuminance * 0.04;
}

void main()
{
	if(ubo.u_Parameters.w < 0.5)
		outFrag = vec4(clamp(panoramaToCubeMap(), 0.0, 50.0), 1.0);
	else if(ubo.u_Parameters.w < 1.5)
		outFrag = vec4(DeGamma(PreethamSky()), 1.0);
	else
		outFrag = vec4(getSky(), 1.0);
}