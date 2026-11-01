#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// One whole sky as an equirectangular picture, baked once per region and read by Cosmo.frag.

layout(location = 0) in vec3 inPosition; // xy = -1..1 across the target
layout(location = 0) out vec4 outColor;  // rgb sky, a = how much fine star dust goes here

layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4 CamSeed; // xyz = noise offset, w = seed
	vec4 Params;  // x = nebula, y = band, z = noise scale, w = band wander
	vec4 Region;  // x = band tightness, y = bulge, z = dust lanes, w = nebula density
	vec4 Pole;    // north pole of the galactic plane
	vec4 Centre;  // towards the galactic centre
	vec4 ColA;
	vec4 ColB;
	vec4 ColC;
	vec4 Tint;
}
ubo;

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

// 4 octaves, not 5. The fifth contributes ~3% of the amplitude and cost a
// fifth of the most expensive shader in the frame - this pass is ~18 ms of a
// 54 ms frame at 1600x900, almost all of it noise.
float fbm(vec3 p)
{
	float a = 0.5;
	float s = 0.0;
	for(int i = 0; i < 4; i++)
	{
		s += a * noise3(p);
		p = p * 2.02 + vec3(0.5);
		a *= 0.5;
	}
	return s;
}

// Cheaper 3-octave variant for secondary fields (hue, dust, detail).
float fbm3(vec3 p)
{
	float a = 0.5;
	float s = 0.0;
	for(int i = 0; i < 3; i++)
	{
		s += a * noise3(p);
		p = p * 2.02 + vec3(0.5);
		a *= 0.5;
	}
	return s;
}

void main()
{
	float lon = inPosition.x * PI;
	float la  = inPosition.y * PI * 0.5;
	vec3 dir  = vec3(cos(la) * cos(lon), sin(la), cos(la) * sin(lon));

	vec3 p = dir * (1.75 * ubo.Params.z) + ubo.CamSeed.xyz + vec3(ubo.CamSeed.w * 7.31);

	// ---- nebulae: domain-warped fbm cut into a few big complexes ----
	float regionM = smoothstep(0.28, 0.56, fbm3(p * 0.32 + vec3(17.3, 5.1, 9.7)) * 1.75);
	vec3 q    = vec3(fbm3(p), fbm3(p + vec3(5.2, 1.3, 2.8)), fbm3(p + vec3(1.7, 9.2, 4.1)));
	float n   = fbm(p + 1.6 * q);
	float low = 0.44 - ubo.Region.w;
	float body = smoothstep(low, low + 0.48, n) * regionM;
	float detail = fbm(p * 3.7 + q * 2.4);
	float dens = body * (0.45 + 0.55 * smoothstep(0.25, 0.85, detail));
	float rim  = body * (1.0 - body) * 4.0;
	rim *= rim;

	float hueSel = clamp(q.y * 1.4 - 0.2, 0.0, 1.0);
	vec3 nebCol = mix(mix(ubo.ColA.rgb, ubo.ColB.rgb, smoothstep(0.25, 0.60, hueSel)),
	                  ubo.ColC.rgb, smoothstep(0.78, 0.97, hueSel));
	nebCol = mix(nebCol, vec3(0.95, 0.85, 0.72), dens * dens * 0.45); // hot core

	vec3 col = nebCol * dens * ubo.Params.x;
	col += ubo.ColB.rgb * rim * dens * 1.4 * ubo.Params.x;

	// ---- Milky Way ----
	vec3 pole   = normalize(ubo.Pole.xyz);
	vec3 centre = normalize(ubo.Centre.xyz - pole * dot(ubo.Centre.xyz, pole));
	vec3 side   = cross(pole, centre);
	vec3 bandP  = dir * 2.3 + ubo.CamSeed.xyz * 0.5;
	float lat   = dot(dir, pole) + (fbm3(bandP) - 0.45) * ubo.Params.w; // uneven edges
	float l     = atan(dot(dir, side), dot(dir, centre));               // 0 at the core
	float toCore = cos(l) * 0.5 + 0.5;

	float disc  = exp(-lat * lat * ubo.Region.x * (1.0 + (1.0 - toCore) * 0.9)); // thins away from the core
	float bulge = exp(-(l * l / 0.30 + lat * lat / 0.045)) * ubo.Region.y;

	vec3 bandQ = dir * 5.0 + ubo.CamSeed.xyz;
	float mott = fbm(bandQ + 1.2 * vec3(fbm3(bandQ * 0.7), fbm3(bandQ * 0.7 + 4.1), 0.0));
	float clouds = 0.30 + 0.70 * smoothstep(0.30, 0.72, mott);

	// Dark lanes: ridged noise down the plane, strongest towards the core (the Great Rift).
	float ridge = 1.0 - abs(2.0 * fbm(dir * 6.5 + ubo.CamSeed.zxy + 2.0 * vec3(mott)) - 1.0);
	float rift  = smoothstep(0.62, 0.92, ridge) * exp(-lat * lat * 220.0) * (0.35 + 0.65 * toCore);
	float fine  = smoothstep(0.34, 0.60, fbm3(dir * 16.0 + ubo.CamSeed.yzx));
	float lane  = (1.0 - rift * ubo.Region.z) * (0.45 + 0.55 * fine);

	vec3 inner = vec3(1.00, 0.84, 0.62);
	vec3 outer = vec3(0.62, 0.72, 0.98);
	vec3 glow  = mix(outer, inner, smoothstep(0.25, 0.95, toCore));
	col += glow * disc * clouds * lane * (0.20 + 0.42 * toCore) * ubo.Params.y;
	col += vec3(1.0, 0.80, 0.56) * bulge * (0.45 + 0.55 * clouds) * lane * 0.75 * ubo.Params.y;

	// Star-forming knots along the plane.
	float knots = smoothstep(0.66, 0.80, fbm(dir * 11.0 + ubo.CamSeed.zyx * 1.3));
	col += vec3(0.95, 0.30, 0.42) * knots * disc * disc * lane * 0.34 * ubo.Params.y;

	col *= ubo.Tint.rgb;
	col += vec3(0.006, 0.009, 0.020) * ubo.Params.x; // never quite black

	float dust = pow(disc, 1.6) * (0.25 + 0.75 * clouds) * (0.35 + 0.65 * lane) + 0.03;
	outColor = vec4(max(col, vec3(0.0)), clamp(dust * ubo.Params.y, 0.0, 1.0));
}
