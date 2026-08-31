#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 colour;

layout (location = 0) in DATA
{
	vec3 worldPos;
	vec2 uv;
	float albedoTid;
	float normalTid;
	vec4 colour;
} fs_in;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

#define MAX_LIGHTS_2D 32

struct Light2D
{
	vec4 Colour;    // rgb, a unused
	vec4 Position;  // xy world, z unused, w radius
	vec4 Direction; // xy spot dir, z height, w type (0 point, 1 spot, 2 global)
	vec4 Params;    // x intensity, y innerAngle(cos), z outerAngle(cos), w falloff
};

layout(set = 0, binding = 1) uniform LightData2D
{
	Light2D u_Lights[MAX_LIGHTS_2D];
	vec4 u_Ambient; // rgb ambient
	ivec4 u_Counts; // x = light count
} lightData;

#define GAMMA 2.2

vec4 SampleTex(int idx, vec2 uv)
{
	switch(idx)
	{
		case 0:  return texture(textures[0],  uv);
		case 1:  return texture(textures[1],  uv);
		case 2:  return texture(textures[2],  uv);
		case 3:  return texture(textures[3],  uv);
		case 4:  return texture(textures[4],  uv);
		case 5:  return texture(textures[5],  uv);
		case 6:  return texture(textures[6],  uv);
		case 7:  return texture(textures[7],  uv);
		case 8:  return texture(textures[8],  uv);
		case 9:  return texture(textures[9],  uv);
		case 10: return texture(textures[10], uv);
		case 11: return texture(textures[11], uv);
		case 12: return texture(textures[12], uv);
		case 13: return texture(textures[13], uv);
		case 14: return texture(textures[14], uv);
		case 15: return texture(textures[15], uv);
	}
	return vec4(1.0);
}

void main()
{
	// Albedo (sRGB -> linear like the unlit batch shader)
	vec4 base = fs_in.colour;
	if(fs_in.albedoTid > 0.0)
	{
		vec4 tex = SampleTex(int(fs_in.albedoTid - 0.5), fs_in.uv);
		base.rgb *= pow(tex.rgb, vec3(GAMMA));
		base.a   *= tex.a;
	}

	// Tangent-space normal. Sprite plane tangent basis is world (X,Y,Z=out of screen).
	vec3 N = vec3(0.0, 0.0, 1.0);
	if(fs_in.normalTid > 0.0)
	{
		vec3 n = SampleTex(int(fs_in.normalTid - 0.5), fs_in.uv).rgb * 2.0 - 1.0;
		N = normalize(n);
	}

	vec3 fragPos = vec3(fs_in.worldPos.xy, 0.0);
	vec3 lighting = lightData.u_Ambient.rgb;

	int count = min(lightData.u_Counts.x, MAX_LIGHTS_2D);
	for(int i = 0; i < count; i++)
	{
		Light2D light = lightData.u_Lights[i];
		float type      = light.Direction.w;
		float intensity = light.Params.x;

		if(type >= 1.5) // global: scene-wide fill, half-lambert so normal maps still show
		{
			vec3 L = normalize(vec3(light.Direction.xy, max(light.Direction.z, 0.001)));
			float ndl = max(dot(N, L), 0.0);
			lighting += light.Colour.rgb * intensity * (0.5 + 0.5 * ndl);
			continue;
		}

		vec3 toL  = vec3(light.Position.xy - fragPos.xy, light.Direction.z);
		float dist = length(toL);
		vec3 L     = toL / max(dist, 0.0001);

		float radius = max(light.Position.w, 0.0001);
		float atten  = clamp(1.0 - dist / radius, 0.0, 1.0);
		atten        = pow(atten, max(light.Params.w, 0.0001));

		float ndl = max(dot(N, L), 0.0);

		float spot = 1.0;
		if(type >= 0.5) // spot cone in the 2D plane
		{
			vec3 spotDir = normalize(vec3(light.Direction.xy, 0.0));
			float cosA   = dot(normalize(vec3(-L.xy, 0.0)), spotDir);
			spot         = smoothstep(light.Params.z, light.Params.y, cosA);
		}

		lighting += light.Colour.rgb * intensity * atten * ndl * spot;
	}

	colour = vec4(base.rgb * lighting, base.a);
}
