#version 450
#include "Common.glslh"
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 outPosition;

layout(set = 0, binding = 1) uniform samplerCube u_CubeMap;
layout(set = 0, binding = 2) uniform UniformBuffer
{
	float Exposure;
	int Mode; //0 env map , 1 custom sky
	float BlurLevel;
	float Time;
	vec4 HorizonColour; // rgb tint at the horizon (y=0), a unused
	vec4 ZenithColour;  // rgb tint at the zenith (y=+1), a unused
	vec4 SunDirection;  // xyz sun direction (normalised), w intensity
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

layout(location = 0) out vec4 outFrag;


vec3 mie(float dist, vec3 sunL)
{
	return max(exp(-pow(dist, 0.25)) * sunL - 0.4, 0.0);
}

// 2D gradient (Perlin-style) noise. Value noise is axis-aligned per cell,
// which produces visible diamond/triangle moiré once you stack octaves —
// gradient noise samples a random direction per lattice point, so the
// resulting field is isotropic and reads as natural cloud breakup.
//
// Dave Hoskins' "hash without sine" — sin()-based hashes lose precision
// catastrophically at large arguments, and the high-frequency FBM octaves
// here push the input range well past sin()'s safe zone (causing aliased,
// polygon-looking artifacts on GPUs that implement sin() with naive range
// reduction). This variant stays accurate for any input magnitude.
vec2 Hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yzx + 33.33);
	return -1.0 + 2.0 * fract((p3.xx + p3.yz) * p3.zy);
}

float GradNoise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	// Quintic Hermite — zero first + second derivatives at cell boundaries.
	vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
	float a = dot(Hash22(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0));
	float b = dot(Hash22(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
	float c = dot(Hash22(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
	float d = dot(Hash22(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));
	// gnoise is in [-1, 1]; remap to [0, 1] for cloud density use.
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y) * 0.5 + 0.5;
}

// Standard FBM (no per-octave rotation — gradient noise is already isotropic
// so rotation just adds aliasing). 5 octaves, freq * 2, amp * 0.5.
float FBM(vec2 p)
{
	float total = 0.0;
	float amp   = 0.5;
	float freq  = 1.0;
	for(int i = 0; i < 5; ++i)
	{
		total += amp * GradNoise(p * freq);
		freq  *= 2.0;
		amp   *= 0.5;
	}
	return total;
}

// Equirectangular-XZ projection. The classic dir.xz/dir.y has a horizon
// singularity that amplifies the screen-quad triangulation into visible
// diagonal facets; sampling dir.xz directly is uniform over the hemisphere
// (compresses toward the zenith, which is fine — clouds calm out overhead).
// Scale sets cloud-cell size in world-ish units.
vec2 CloudUV(vec3 dir, float speed)
{
	const float SKY_SCALE = 6.0;
	vec2 uv = dir.xz * SKY_SCALE;
	vec2 wind = vec2(data.CloudWindDir.x, data.CloudWindDir.z) * speed * data.Time;
	return uv + wind;
}

// Stylised cumulus (hand-drawn cartoon). Two billow scales build rounded
// puffs, a crisp narrow edge gives the bold outline, and the shading is
// posterised into a few flat bands so it reads like cel-shaded artwork
// rather than a smooth gradient. CloudColour.a drives the dark rim/outline.
vec4 CloudsStylised(vec3 dir, vec3 sunDir)
{
	vec2 uv = CloudUV(dir, data.CloudParams.z);
	// Big rounded billows + a touch of higher-freq break-up.
	float baseN  = FBM(uv);
	float detail = FBM(uv * 2.3 + 11.0);
	float n      = baseN * 0.78 + detail * 0.22;

	float coverage = data.CloudParams.x;
	float cut      = mix(0.62, 0.20, coverage);

	// Crisp puffy core — narrow smoothstep for the hard cartoon edge.
	float core    = smoothstep(cut, cut + 0.05, n);
	// Outline band hugging the core, used as a drawn rim.
	float outline = smoothstep(cut - 0.07, cut, n) * (1.0 - core);
	float alpha   = clamp(core + outline, 0.0, 1.0) * data.CloudParams.y;

	// Fake form shading: brighter toward the cloud "top" (dense centre) and
	// toward the sun, then quantise to flat cel bands.
	float form    = smoothstep(cut, 0.95, n);
	float lightDot = clamp(dot(normalize(dir), sunDir) * 0.5 + 0.5, 0.0, 1.0);
	float lit     = mix(0.55, 1.0, form) * mix(0.78, 1.0, lightDot);
	lit           = floor(lit * 3.0 + 0.5) / 3.0; // 3-step toon ramp

	vec3 body = data.CloudColour.rgb * lit;
	vec3 rim  = data.CloudColour.rgb * (1.0 - data.CloudColour.a); // dark outline
	vec3 cloud = mix(rim, body, core);

	float horizonFade = smoothstep(0.04, 0.28, dir.y);
	alpha *= horizonFade;
	return vec4(cloud, alpha);
}

// Realistic stratocumulus: domain-warped FBM for organic shapes, a higher-freq
// erosion pass for wispy boundaries, and a Beer–Powder lighting model that
// fakes self-shadowing depth. A tight forward-scatter lobe gives the silver
// lining when looking toward the sun.
vec4 CloudsRealistic(vec3 dir, vec3 sunDir)
{
	vec2 uv = CloudUV(dir, data.CloudParams.z);
	// Domain warp — two channels of low-freq FBM offset the main lookup so
	// shapes flow instead of sitting on a grid.
	vec2 warp = vec2(FBM(uv * 0.5 + 5.2), FBM(uv * 0.5 + 9.7));
	float n   = FBM(uv + warp * 0.6);
	// Erode edges with detail noise — eats into low-density rims for wisps.
	float detail = FBM(uv * 3.1 + 21.0);
	n = clamp(n - (1.0 - n) * detail * 0.35, 0.0, 1.0);

	float coverage = data.CloudParams.x;
	float cut      = mix(0.70, 0.20, coverage);
	float density  = smoothstep(cut, cut + 0.30, n) * data.CloudParams.y;

	// Beer–Powder: density stands in for optical depth. Beer darkens dense
	// cores, the powder term restores the dark-edge falloff that pure Beer
	// misses, together giving believable volumetric shading.
	float beer   = exp(-density * 3.0);
	float powder = 1.0 - exp(-density * 4.0);
	float energy = clamp(beer * powder * 2.0, 0.0, 1.0);

	// Forward scatter toward the sun — tight lobe (silver lining) + broad rim.
	float sunDot = max(dot(normalize(dir), sunDir), 0.0);
	float silver = pow(sunDot, 8.0) + pow(sunDot, 3.0) * 0.35;

	vec3 shadowCol = data.CloudColour.rgb * (1.0 - data.CloudColour.a);
	vec3 litCol    = mix(data.CloudColour.rgb, vec3(1.0, 0.96, 0.88), 0.3);
	vec3 cloud     = mix(shadowCol, litCol, energy);
	cloud         += vec3(1.0, 0.9, 0.75) * silver * density;

	float horizonFade = smoothstep(0.04, 0.28, dir.y);
	float alpha = clamp(density * horizonFade, 0.0, 1.0);
	return vec4(cloud, alpha);
}

// Aurora borealis — animated curtains in the upper sky. Seamless sky-plane
// projection (avoids the atan azimuth seam), three layered curtains for depth,
// each warped by flowing FBM so the rays ripple. Colour ramps from the base
// tint at the bottom to a magenta tip up high, the classic aurora gradient.
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

// Cheap 3D hash for star placement — quantises the view direction onto a
// grid so each cell either contains a star (above the sparsity threshold)
// or doesn't. Density param controls threshold; brightness scales the
// resulting point. Twinkle uses Time + per-cell phase so adjacent stars
// don't pulse in sync.
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

	// Two layered grids (different cell sizes) — looks denser without each
	// cell needing a fine threshold.
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

		// Threshold drives sparsity. Higher density -> lower threshold ->
		// more cells contain a star. Soft falloff for antialiasing.
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

// Per-planet procedural sky.
// Mixes HorizonColour -> ZenithColour by view.y, then adds a soft sun glow
// in the SunDirection. Optional cloud dome + horizon fog tint applied on top.
vec3 GetSky()
{
	vec3 uv = normalize(outPosition.xyz);

	float h = clamp(uv.y, 0.0, 1.0);
	// Two-stop gradient with an extra horizon-haze lift. The zenith->horizon
	// mix uses a 2.5 power so the rich zenith colour holds across most of the
	// dome instead of washing toward the pale horizon too early.
	float horizonBlend = pow(1.0 - h, 2.5);
	vec3 base = mix(data.ZenithColour.rgb, data.HorizonColour.rgb, horizonBlend);

	// Below-horizon: fade toward a slightly darker horizon tone (the ground side).
	float belowFade = clamp(-uv.y * 2.0, 0.0, 1.0);
	base = mix(base, data.HorizonColour.rgb * 0.35, belowFade);

	vec3 sunDir = normalize(data.SunDirection.xyz);
	float sunIntensity = data.SunDirection.w;
	float sunDist = length(uv - sunDir);

	// Sun disc + tighter halo (broad halo is the main washed-out culprit).
	float disc = smoothstep(0.04, 0.0, sunDist);
	float halo = exp(-sunDist * 8.0) * 0.4;
	vec3 sun = (disc + halo) * sunIntensity * vec3(1.0, 0.95, 0.85);

	// Mie-style horizon scattering boost when sun is low.
	vec3 mieGlow = mie(sunDist, vec3(1.0, 0.85, 0.6)) * horizonBlend * sunIntensity * 0.4;

	vec3 sky = base + sun + mieGlow;

	// How dark the underlying sky reads — drives night-only effects (stars,
	// aurora) so they fade out automatically in daylight without a toggle.
	float skyLuma  = dot(base, vec3(0.2126, 0.7152, 0.0722));
	float darkness = clamp(1.0 - skyLuma / 0.35, 0.0, 1.0);

	// Aurora — additive glow beneath clouds, fades with daylight.
	if(data.AuroraParams.x > 0.001)
		sky += Aurora(uv) * darkness;

	// Stars composited under clouds/fog so atmospheric effects can occlude
	// them. Visibility scales with how dark the underlying sky tint is,
	// gated by StarParams.w threshold — daytime skies suppress stars
	// automatically (no per-time-of-day toggle needed).
	if(data.StarParams.x > 0.001)
	{
		float starDark = clamp(1.0 - skyLuma / max(data.StarParams.w, 0.001), 0.0, 1.0);
		float aboveHorizon = smoothstep(-0.02, 0.10, uv.y);
		// Suppress stars near the sun disc.
		float sunMask = 1.0 - smoothstep(0.0, 0.25, sunDist);
		float starMask = starDark * aboveHorizon * (1.0 - sunMask * 0.9);
		sky += Stars(uv) * starMask;
	}

	// Clouds — composite over sky if enabled.
	if(data.CloudParams.w >= 0.0)
	{
		vec4 cloud;
		if(data.CloudParams.w < 0.5)
			cloud = CloudsStylised(uv, sunDir);
		else
			cloud = CloudsRealistic(uv, sunDir);
		// Hide clouds under horizon to avoid them painting the "ground" half.
		float skyMask = smoothstep(0.0, 0.08, uv.y);
		cloud.a *= skyMask;
		sky = mix(sky, cloud.rgb, cloud.a);
	}

	// Horizon fog tint — pull lowest portion of sky toward FogColour so
	// scene fog blends seamlessly with the dome. Fixed band 0..0.25 of view.y
	// (just above horizon). FogColour.a scales overall strength.
	if(data.FogColour.a > 0.001)
	{
		float fh = 1.0 - clamp(uv.y / 0.25, 0.0, 1.0);
		float mixAmt = fh * fh * data.FogColour.a;
		sky = mix(sky, data.FogColour.rgb, mixAmt);
	}

	// Gentle saturation lift — procedural sky reads washed out after the
	// tonemapper compresses it, so push colour purity back in a touch.
	float luma = dot(sky, vec3(0.2126, 0.7152, 0.0722));
	sky = mix(vec3(luma), sky, 1.15);

	return sky;
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


