#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 outColour;

layout (location = 0) in DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 colour;
	vec4 outlineColour;
	float outlineWidth;
} fs_in;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float scaleFactor = 2;
float thickness = 0.5;

float ScreenPxRange()
{
    vec2 unitRange = vec2(scaleFactor)/vec2(textureSize(textures[int(fs_in.tid)], 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(fs_in.uv);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

void main()
{
	vec4 mainColour = fs_in.colour;
	vec4 outlineColour = fs_in.outlineColour;

	float screenPxRange = ScreenPxRange();
	float outlineWidth = fs_in.outlineWidth;

	vec4 msd = texture(textures[int(fs_in.tid)], fs_in.uv);
    float sd = median(msd.r, msd.g, msd.b);

    float screenPxDistance = screenPxRange * (sd - 0.5 - (outlineWidth));
    float screenPxDistanceOutline = screenPxRange * (sd - 0.5);

    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    float outlineOp = clamp(screenPxDistanceOutline + 0.5, 0.0, 1.0);

	vec4 colour = outlineWidth == 0 ? mainColour : mix(outlineColour, mainColour, outlineWidth < 0 ? outlineOp : opacity);
    colour.a *= max(opacity, outlineOp);

	//float width = fwidth(sd);
    //float alpha = smoothstep(0.5 - width, 0.5 + width, sd);
	//colour.a   *= alpha;
    outColour   = colour;
}