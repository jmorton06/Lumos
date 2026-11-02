#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 colour;

layout (location = 0) in DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 colour;
	vec2 rectSize;
	float cornerRadius;
} fs_in;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

#define GAMMA 2.2
#define DISABLE_GAMMA_CORRECTION 0

vec4 GammaCorrectTexture(vec4 samp)
{
#if DISABLE_GAMMA_CORRECTION
	return samp;
#endif
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

// SDF rounded rectangle - returns distance to edge
float sdRoundedRect(vec2 p, vec2 halfSize, float radius)
{
	vec2 q = abs(p) - halfSize + radius;
	return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

void main()
{
	vec4 texColor = fs_in.colour;
	// slots offset by 100 are UI images: already display colours, so no linearising
	float tid = fs_in.tid;
	bool raw  = tid > 99.5;
	if (raw)
		tid -= 100.0;
    if (tid > 0.0)
    {
		vec4 samp = vec4(1.0);
		switch(int(tid - 0.5))
		{
			case 0: samp = texture(textures[0], fs_in.uv); break;
			case 1: samp = texture(textures[1], fs_in.uv); break;
			case 2: samp = texture(textures[2], fs_in.uv); break;
			case 3: samp = texture(textures[3], fs_in.uv); break;
			case 4: samp = texture(textures[4], fs_in.uv); break;
			case 5: samp = texture(textures[5], fs_in.uv); break;
			case 6: samp = texture(textures[6], fs_in.uv); break;
			case 7: samp = texture(textures[7], fs_in.uv); break;
			case 8: samp = texture(textures[8], fs_in.uv); break;
			case 9: samp = texture(textures[9], fs_in.uv); break;
			case 10: samp = texture(textures[10], fs_in.uv); break;
			case 11: samp = texture(textures[11], fs_in.uv); break;
			case 12: samp = texture(textures[12], fs_in.uv); break;
			case 13: samp = texture(textures[13], fs_in.uv); break;
			case 14: samp = texture(textures[14], fs_in.uv); break;
			case 15: samp = texture(textures[15], fs_in.uv); break;
		}
		texColor *= raw ? samp : GammaCorrectTexture(samp);
    }

	// Apply rounded corners if radius > 0
	if(fs_in.cornerRadius > 0.0 && fs_in.rectSize.x > 0.0)
	{
		vec2 localPos = (fs_in.uv - 0.5) * fs_in.rectSize;
		vec2 halfSize = fs_in.rectSize * 0.5;

		float radius = min(fs_in.cornerRadius, min(halfSize.x, halfSize.y));

		float dist = sdRoundedRect(localPos, halfSize, radius);

		float alpha = 1.0 - smoothstep(-1.5, 0.5, dist);
		texColor.a *= alpha;
	}
	else if(fs_in.cornerRadius < 0.0 && fs_in.rectSize.x > 0.0)
	{
		vec2 localPos = (fs_in.uv - 0.5) * fs_in.rectSize;
		vec2 halfSize = fs_in.rectSize * 0.5;

		float radius = min(halfSize.x, halfSize.y);
		float dist   = sdRoundedRect(localPos, halfSize, radius);
		float stroke = max(1.0, -fs_in.cornerRadius);

		// Keep only the band [-stroke, 0] inside the shape edge.
		float alpha = (1.0 - smoothstep(-1.0, 0.5, dist))
		            * smoothstep(-stroke - 1.0, -stroke + 0.5, dist);
		texColor.a *= alpha;
	}

	colour = texColor;
}
