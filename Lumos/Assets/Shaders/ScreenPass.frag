#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
	float Exposure;
	float BloomIntensity;
	 int ToneMapIndex;
	float p1;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;

const float gamma = 2.2;

vec3 GammaCorrect(vec3 color, float gamma)
{
	return pow(color, vec3(1.0f / gamma));
}

// Based on http://www.oscars.org/science-technology/sci-tech-projects/aces
vec3 ACESTonemap(vec3 color)
{
	mat3 m1 = mat3(
				   0.59719, 0.07600, 0.02840,
				   0.35458, 0.90834, 0.13383,
				   0.04823, 0.01566, 0.83777
				   );
	mat3 m2 = mat3(
				   1.60475, -0.10208, -0.00327,
				   -0.53108, 1.10813, -0.07276,
				   -0.07367, -0.00605, 1.07602
				   );
	vec3 v = m1 * color;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

vec3 linearToneMapping(vec3 color)
{
	float exposure = 1.;
	color = clamp(exposure * color, 0., 1.);
	return color;
}

vec3 simpleReinhardToneMapping(vec3 color)
{
	float exposure = 1.5;
	color *= exposure/(1. + color / exposure);
	return color;
}

vec3 lumaBasedReinhardToneMapping(vec3 color)
{
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma / (1. + luma);
	color *= toneMappedLuma / luma;
	return color;
}

vec3 whitePreservingLumaBasedReinhardToneMapping(vec3 color)
{
	float white = 2.;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / luma;
	return color;
}

vec3 RomBinDaHouseToneMapping(vec3 color)
{
    color = exp( -1.0 / ( 2.72*color + 0.15 ) );
	return color;
}

vec3 filmicToneMapping(vec3 color)
{
	color = max(vec3(0.), color - vec3(0.004));
	color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
	return color;
}

vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	return color;
}

void main()
{
	vec3 colour = texture(u_Texture, outTexCoord).rgb;
	colour *= ubo.Exposure;
	
	int i = ubo.ToneMapIndex;
	if (i == 1) colour = linearToneMapping(colour);
	if (i == 2) colour = simpleReinhardToneMapping(colour);
	if (i == 3) colour = lumaBasedReinhardToneMapping(colour);
	if (i == 4) colour = whitePreservingLumaBasedReinhardToneMapping(colour);
	if (i == 5) colour = RomBinDaHouseToneMapping(colour);		
	if (i == 6) colour = filmicToneMapping(colour);
	if (i == 7) colour = Uncharted2ToneMapping(colour);
	if (i == 8) colour = ACESTonemap(colour);
	
	//colour = pow(colour, vec3(1. / gamma));
	outFrag = vec4(colour, 1.0);
}