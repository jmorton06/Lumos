#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "EnvironmentMapping.glslh"

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform samplerCube u_Texture;
layout(set = 0, binding = 1) uniform UniformBuffer
{
	uint Samples;
}
ubo;

layout(push_constant) uniform PushConsts
{
	uint cubeFaceIndex;
} pushConsts;

layout(location = 0) out vec4 outFrag;

// vec3 GetCubeMapTexCoord()
// {
// 	vec2 uv = outTexCoord;
// 	uv = 2.0 * vec2(uv.x, 1.0 - uv.y) - vec2(1.0);
	
//     vec3 ret = vec3(0.0, 0.0, 0.0);
// 	int face = int(pushConsts.cubeFaceIndex);

//     if (face == 0)      ret = vec3(  1.0, uv.y, -uv.x);
//     else if (face == 1) ret = vec3( -1.0, uv.y,  uv.x);
//     else if (face == 2) ret = vec3( uv.x,  1.0, uv.y);
//     else if (face == 3) ret = vec3( uv.x, -1.0,  -uv.y);
//     else if (face == 4) ret = vec3( uv.x, uv.y,   1.0);
//     else ret = vec3(-uv.x, uv.y,  -1.0); //if (face == 5)
//     return normalize(ret);
// }

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

void main()
{
	vec3 N = normalize(GetCubeMapTexCoord(int(pushConsts.cubeFaceIndex), outTexCoord));
	vec3 S, T;
	ComputeBasisVectors(N, S, T);

	uint samples = 64 * ubo.Samples;

	// Monte Carlo integration of hemispherical irradiance.
	// As a small optimization this also includes Lambertian BRDF assuming perfectly white surface (albedo of 1.0)
	// so we don't need to normalize in PBR fragment shader (so technically it encodes exitant radiance rather than irradiance).
	vec3 irradiance = vec3(0);
	for(uint i = 0; i < samples; i++)
	{
		vec2 u  = SampleHammersley(i, samples);
		vec3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N, S, T);
		float cosTheta = max(0.0, dot(Li, N));

		// PIs here cancel out because of division by pdf.
		irradiance += 2.0 * textureLod(u_Texture, Li, 0).rgb * cosTheta;
	}
	irradiance /= vec3(samples);

	outFrag = vec4(irradiance, 1.0);
}