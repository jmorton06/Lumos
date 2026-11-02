#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// The sky picture from CosmoBake.frag (two while a new region fades in) + fine star dust.

layout(location = 0) in vec3 inPosition; // view ray (not normalised)
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4 Params; // x = fade towards SkyA, y = gain
}
ubo;

layout(set = 0, binding = 2) uniform sampler2D SkyA;
layout(set = 0, binding = 3) uniform sampler2D SkyB;

const float PI = 3.14159265;

float hash(vec3 x)
{
	x = fract(x * 0.3183099 + vec3(0.1, 0.2, 0.3));
	x *= 17.0;
	return fract(x.x * x.y * x.z * (x.x + x.y + x.z));
}

float noise3(vec3 x)
{
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash(i),                  hash(i + vec3(1, 0, 0)), f.x),
	               mix(hash(i + vec3(0, 1, 0)),  hash(i + vec3(1, 1, 0)), f.x), f.y),
	           mix(mix(hash(i + vec3(0, 0, 1)),  hash(i + vec3(1, 0, 1)), f.x),
	               mix(hash(i + vec3(0, 1, 1)),  hash(i + vec3(1, 1, 1)), f.x), f.y),
	           f.z);
}

void main()
{
	vec3 dir = normalize(inPosition);
	vec2 uv  = vec2(atan(dir.z, dir.x) / (2.0 * PI) + 0.5, asin(clamp(dir.y, -1.0, 1.0)) / PI + 0.5);

	// Lod 0: the longitude wraps at the seam, so derivatives would pick a tiny mip there.
	vec4 sky = textureLod(SkyA, uv, 0.0);
	if(ubo.Params.x < 0.999)
		sky = mix(textureLod(SkyB, uv, 0.0), sky, ubo.Params.x);

	vec3 col = sky.rgb;
	if(sky.a > 0.02)
	{
		float s1 = pow(max(noise3(dir * 300.0) - 0.60, 0.0) * 2.5, 3.0);
		col += vec3(0.82, 0.84, 0.92) * s1 * 0.55 * sky.a;
	}
	col *= ubo.Params.y;

	// Dither: the sky is mostly deep gradients, which band hard at 8 bits.
	col += (hash(dir * 913.7) - 0.5) * (1.5 / 255.0);

	outColor = vec4(max(col, vec3(0.0)), 1.0);
}
