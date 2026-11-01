#version 450
#include "Common.glslh"
#include "Sky.glslh"
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UBO
{
	mat4 invProjection;
	mat4 invView;
	vec4 SunDirection; // xyz towards the sun
	vec4 SunColour;    // rgb illuminance (lux / 120000)
	vec4 HorizonTint;
	vec4 ZenithTint;
	vec4 Params;       // x > 0.5: procedural sky
} ubo;

layout(location = 0) out vec3 outPosition;
// The air is smooth: it is worked out here, at the corners of a grid over the screen, and
// the fragment shader only adds what has detail (clouds, sun, stars). Per pixel it cost
// most of the frame.
layout(location = 1) out vec3 outSky;
layout(location = 2) out vec3 outThrough;
layout(location = 3) flat out vec3 outSunLight;
layout(location = 4) flat out vec3 outAmbient;

out gl_PerVertex
{
    vec4 gl_Position;
};

// keep in step with kSkyGridX / kSkyGridY in SceneRenderer.cpp
const int GRID_X = 128;
const int GRID_Y = 72;

void main()
{
	const ivec2 corners[6] = ivec2[6](ivec2(0, 0), ivec2(1, 0), ivec2(1, 1), ivec2(0, 0), ivec2(1, 1), ivec2(0, 1));
	int quad   = gl_VertexIndex / 6;
	ivec2 cell = ivec2(quad % GRID_X, quad / GRID_X) + corners[gl_VertexIndex % 6];

	// depth 0 = the far plane (reversed-Z)
	vec4 pos = vec4(vec2(cell) / vec2(GRID_X, GRID_Y) * 2.0 - 1.0, 0.0, 1.0);
	gl_Position = pos;

	outPosition = mat3(ubo.invView) * vec3(ubo.invProjection * pos);

	outSky      = vec3(0.0);
	outThrough  = vec3(1.0);
	outSunLight = vec3(0.0);
	outAmbient  = vec3(0.0);
	if(ubo.Params.x > 0.5)
	{
		SkyParams sky;
		sky.SunDir       = normalize(ubo.SunDirection.xyz);
		sky.SunE         = ubo.SunColour.rgb;
		sky.HorizonTint  = ubo.HorizonTint.rgb;
		sky.ZenithTint   = ubo.ZenithTint.rgb;
		sky.CloudColour  = vec4(0.0);
		sky.CloudParams  = vec4(0.0, 0.0, 0.0, -1.0);
		sky.CloudWindDir = vec4(0.0);
		sky.Time         = 0.0;

		outSky      = SkyAtmosphere(normalize(outPosition), sky, 8, outThrough);
		// the same everywhere, and flat: only a triangle's first vertex is read
		if(gl_VertexIndex % 3 == 0)
		{
			outSunLight = SkySunAtGround(sky);
			vec3 unused;
			outAmbient  = SkyAtmosphere(vec3(0.0, 1.0, 0.0), sky, 4, unused);
		}
	}
}
