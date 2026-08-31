#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in vec3 inPosition; // view ray (not normalised)
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4 CamSeed; // xyz = camera pos offset (pre-scaled), w = seed
	vec4 Params;  // x = nebula intensity, y = band intensity, z = noise scale, w unused
}
ubo;

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

float fbm(vec3 p)
{
	float a = 0.5;
	float s = 0.0;
	for(int i = 0; i < 5; i++)
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
	vec3 dir = normalize(inPosition);
	vec3 p   = dir * (2.5 * ubo.Params.z) + ubo.CamSeed.xyz + vec3(ubo.CamSeed.w * 7.31);

	// ---- nebulae ----
	// Domain-warped fbm, thresholded to sparse patches. The old look failed as
	// uniform low-contrast fog: threshold raised so most of the sky is genuinely
	// dark, and a high-frequency detail field breaks the interior into wisps.
	vec3 q  = vec3(fbm(p), fbm(p + vec3(5.2, 1.3, 2.8)), fbm(p + vec3(1.7, 9.2, 4.1)));
	float n = fbm(p + 1.6 * q);
	float body = smoothstep(0.55, 0.95, n);

	float detail = fbm3(p * 3.7 + q * 2.4);
	float dens   = body * (0.45 + 0.55 * smoothstep(0.25, 0.85, detail));

	// Rim light where density crosses the threshold - shape instead of smoke.
	float rim = body * (1.0 - body) * 4.0;
	rim *= rim;

	// Hue rides the warp field, so colour follows structure rather than sitting
	// in unrelated blobs. Saturated bases; bright cores pull toward warm white.
	float hueSel = clamp(q.y * 1.4 - 0.2, 0.0, 1.0);
	vec3 colA = vec3(0.42, 0.16, 0.58); // violet
	vec3 colB = vec3(0.10, 0.34, 0.58); // deep blue-teal
	vec3 colC = vec3(0.62, 0.22, 0.12); // ember
	vec3 nebCol = mix(mix(colA, colB, smoothstep(0.25, 0.60, hueSel)),
	                  colC, smoothstep(0.78, 0.97, hueSel));
	nebCol = mix(nebCol, vec3(0.95, 0.85, 0.72), dens * dens * 0.45); // hot core

	vec3 col = nebCol * dens * ubo.Params.x;
	col += vec3(0.30, 0.45, 0.62) * rim * dens * 0.8 * ubo.Params.x;

	// ---- Milky Way ----
	// Real galactic plane in equatorial J2000 (the HYG/world frame).
	const vec3 NGP = vec3(-0.8676, -0.1980, 0.4560);  // north galactic pole
	const vec3 GC  = vec3(-0.0546, -0.8728, -0.4851); // galactic centre
	float lat  = dot(dir, NGP);                        // sin(galactic latitude)
	float band = exp(-lat * lat * 45.0);
	float centre = 0.35 + 0.65 * pow(max(dot(dir, GC), 0.0) * 0.5 + 0.5, 3.0);

	// Star clouds and DARK dust lanes: the lanes multiply the glow down hard,
	// which is what makes the band read as a structure instead of a stripe.
	float clouds = fbm3(dir * 7.0 + vec3(3.1, 7.7, 1.9));
	float lanes  = fbm3(dir * 16.0 + vec3(9.4, 2.2, 5.6));
	float lane   = smoothstep(0.30, 0.62, lanes);           // 0 = deep lane
	float bandI  = band * centre * (0.30 + 0.70 * smoothstep(0.30, 0.75, clouds))
	             * (0.25 + 0.75 * lane);
	col += vec3(0.78, 0.80, 0.88) * bandI * ubo.Params.y * 0.30;
	// Warm glow around the galactic core.
	col += vec3(0.85, 0.70, 0.50) * band * pow(max(dot(dir, GC), 0.0), 6.0)
	     * ubo.Params.y * 0.22;

	// Unresolved star dust: fine grain hugging the plane, clumped by the same
	// cloud field as the glow so it pools where the band is bright instead of
	// sprinkling evenly. Soft, low amplitude - the real HYG stars sit on top.
	float dustW = pow(band, 1.6) * (0.25 + 0.75 * smoothstep(0.35, 0.8, clouds)) + 0.03;
	float s1 = pow(max(noise3(dir * 340.0) - 0.60, 0.0) * 2.5, 3.0);
	float s2 = pow(max(noise3(dir * 150.0 + vec3(4.7)) - 0.58, 0.0) * 2.4, 3.0);
	col += vec3(0.82, 0.84, 0.92) * (s1 * 0.45 + s2 * 0.25) * dustW
	     * (0.35 + 0.65 * lane) * ubo.Params.y;

	// ---- regional tint + floor ----
	vec3 rp = ubo.CamSeed.xyz * 0.13 + vec3(ubo.CamSeed.w);
	vec3 regionT = normalize(vec3(0.9, 0.9, 1.0) + 0.35 * vec3(
		sin(rp.x * 1.7 + rp.z * 0.9),
		sin(rp.y * 2.3 + 1.8),
		sin(rp.z * 1.3 + 3.9)));
	col = mix(col, col * regionT * 1.72, 0.22);

	col += vec3(0.006, 0.009, 0.020) * ubo.Params.x;

	// Dither: the sky is mostly deep gradients, which band hard at 8 bits.
	col += (hash(dir * 913.7) - 0.5) * (1.5 / 255.0);

	outColor = vec4(max(col, vec3(0.0)), 1.0);
}
