#version 450
#include "Common.glslh"
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 outPosition;
// worked out per vertex (Skybox.vert): the air, what is left of things behind it, and the
// light the clouds are in
layout(location = 1) in vec3 outSky;
layout(location = 2) in vec3 outThrough;
layout(location = 3) flat in vec3 outSunLight;
layout(location = 4) flat in vec3 outAmbient;

layout(set = 0, binding = 1) uniform samplerCube u_CubeMap;
layout(set = 0, binding = 2) uniform UniformBuffer
{
	float Exposure;
	int Mode; //0 env map , 1 custom sky
	float BlurLevel;
	float Time;
	vec4 HorizonColour; // rgb multiplier on the sky near the horizon (1 = untouched)
	vec4 ZenithColour;  // rgb multiplier overhead
	vec4 SunDirection;  // xyz towards the sun
	vec4 SunColour;     // rgb illuminance above the atmosphere (lux / 120000)
	vec4 FogColour;     // rgb tint, a strength (0..1)
	vec4 FogParams;     // x density (unused here), y heightFalloff (unused),
	                    // z linearStart, w linearEnd (used for horizon mix curve)
	vec4 CloudColour;   // rgb tint, a shadow strength
	vec4 CloudParams;   // x coverage(0..1), y density(0..1), z speed, w styleMode (-1 off, 0 stylised, 1 realistic)
	vec4 CloudWindDir;  // xz wind 2D, y altitude band (0..1), w thickness boost
	vec4 StarParams;    // x density(0..1, 0 off), y brightness, z twinkle speed, w sky-luma threshold
	vec4 StarColour;    // rgb tint, a unused
	vec4 AuroraColour;  // rgb base tint (green), a tip-hue blend toward magenta (0..1)
	vec4 AuroraParams;  // x intensity (0 off), y vertical centre, z flow speed, w band width
} data;

layout(set = 0, binding = 3) uniform sampler2D u_SkyNoise; // tiling cloud noise

layout(location = 0) out vec4 outFrag;

#define SKY_NOISE u_SkyNoise
#include "Sky.glslh"


// 0..1, for the aurora
float FBM(vec2 p)
{
	return SkyFBM(p, 5);
}

vec3 Aurora(vec3 dir)
{
	float intensity = data.AuroraParams.x;
	if(intensity <= 0.001) return vec3(0.0);

	float vis = smoothstep(0.0, 0.35, dir.y); // upper sky only
	if(vis <= 0.0) return vec3(0.0);

	float t      = data.Time * data.AuroraParams.z;
	float centre = data.AuroraParams.y;
	float width  = max(data.AuroraParams.w, 0.05);

	// Seamless projection onto the sky plane (no azimuth wrap).
	vec2 p = dir.xz / max(dir.y + 0.25, 0.25);

	vec3 tip = mix(data.AuroraColour.rgb, vec3(0.75, 0.2, 0.95), data.AuroraColour.a);

	vec3 col = vec3(0.0);
	for(int i = 0; i < 3; ++i)
	{
		float fi = float(i);
		// Flowing horizontal warp so the curtain wavers.
		float warp   = FBM(p * 1.5 + vec2(t * 0.3 + fi * 4.0, fi)) - 0.5;
		// Narrow vertical streaks along one axis = curtain rays.
		float streak = FBM(vec2(p.x * 2.5 + warp * 3.0 + fi * 2.0, t * 0.15 + fi));
		streak = pow(smoothstep(0.45, 0.95, streak), 1.5);

		// Place + rayed vertical falloff (Gaussian band, slight per-layer rise).
		float band  = (dir.y - centre - fi * 0.06) / width;
		float vFall = exp(-band * band * 2.5);
		float ray   = streak * vFall;

		float up = clamp(band * 0.5 + 0.5, 0.0, 1.0);
		col += mix(data.AuroraColour.rgb, tip, up) * ray;
	}
	return col * intensity * vis;
}

float Hash13(vec3 p)
{
	p = fract(p * 0.1031);
	p += dot(p, p.yzx + 33.33);
	return fract((p.x + p.y) * p.z);
}

vec3 Stars(vec3 dir)
{
	float density = data.StarParams.x;
	if(density <= 0.001) return vec3(0.0);

	vec3 total = vec3(0.0);
	for(int layer = 0; layer < 2; ++layer)
	{
		float scale = (layer == 0) ? 250.0 : 110.0;
		vec3 g = dir * scale;
		vec3 cell = floor(g);
		// Random subpixel offset within cell so stars don't grid-align.
		vec3 hOff = vec3(Hash13(cell + 1.7), Hash13(cell + 8.3), 0.0);
		vec3 local = fract(g) - 0.5 - (hOff - 0.5) * 0.6;
		float dist = length(local.xy);

		float starProb = Hash13(cell);
		float thresh = mix(0.997, 0.965, density);
		float gate = step(thresh, starProb);
		float starMag = smoothstep(0.18, 0.0, dist) * gate;

		// Twinkle: per-star phase * time. Skip the sin() if speed==0.
		float twinkle = 1.0;
		if(data.StarParams.z > 0.001)
		{
			float phase = starProb * 62.83 + data.Time * data.StarParams.z * 3.0;
			twinkle = 0.5 + 0.5 * sin(phase);
			twinkle = mix(0.45, 1.0, twinkle);
		}

		total += vec3(starMag * twinkle);
	}
	return total * data.StarColour.rgb * data.StarParams.y;
}

vec3 GetSky()
{
	vec3 dir = normalize(outPosition.xyz);

	SkyParams sky;
	sky.SunDir       = normalize(data.SunDirection.xyz);
	sky.SunE         = data.SunColour.rgb;
	sky.HorizonTint  = data.HorizonColour.rgb;
	sky.ZenithTint   = data.ZenithColour.rgb;
	sky.CloudColour  = data.CloudColour;
	sky.CloudParams  = data.CloudParams;
	sky.CloudWindDir = data.CloudWindDir;
	sky.Time         = data.Time;

	vec3 through = outThrough;
	vec3 colour  = SkyClouds(dir, sky, outSky, outSunLight, outAmbient, through);

	// The disc: twice the real width so it reads at any resolution, and nothing like the
	// real brightness (about 15000 x the illuminance) - that would swamp bloom and exposure.
	float sunCos = dot(dir, sky.SunDir);
	float disc   = smoothstep(cos(0.0105), cos(0.0085), sunCos);
	// darker towards the rim
	float limb   = 0.55 + 0.45 * sqrt(clamp((sunCos - cos(0.0105)) / (1.0 - cos(0.0105)), 0.0, 1.0));
	colour += sky.SunE * through * disc * limb * 40.0;

	// stars and aurora belong to the night
	float sunLuma = dot(sky.SunE, vec3(0.2126, 0.7152, 0.0722));
	float day     = smoothstep(-0.14, 0.04, sky.SunDir.y) * clamp(sunLuma * 20.0, 0.0, 1.0);

	if(data.AuroraParams.x > 0.001)
		colour += Aurora(dir) * (1.0 - day) * through;

	if(data.StarParams.x > 0.001)
	{
		float starDark     = clamp(1.0 - day * 0.35 / max(data.StarParams.w, 0.001), 0.0, 1.0);
		float aboveHorizon = smoothstep(-0.02, 0.10, dir.y);
		colour += Stars(dir) * starDark * aboveHorizon * through;
	}

	if(data.FogColour.a > 0.001)
	{
		float fh     = 1.0 - clamp(dir.y / 0.25, 0.0, 1.0);
		float mixAmt = fh * fh * data.FogColour.a;
		colour = mix(colour, data.FogColour.rgb, mixAmt);
	}

	return colour;
}

void main()
{
	vec3 colour = vec3(1.0, 0.0,0.0);

	if(data.Mode == 0)
	{
		colour = textureLod(u_CubeMap, outPosition, data.BlurLevel).xyz;
	}
	else
	{
		colour = GetSky();
	}

	colour *= data.Exposure;
	outFrag = vec4(colour, 1.0);
}
