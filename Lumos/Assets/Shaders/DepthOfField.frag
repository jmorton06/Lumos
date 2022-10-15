#version 450
#include "Common.glslh"
//https://blog.voxagon.se/2018/05/04/bokeh-depth-of-field-in-single-pass.html

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
	vec2 DOFParams;
	vec2 DepthUnpackConsts;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(set = 0, binding = 2) uniform sampler2D u_DepthTexture;
layout(location = 0) out vec4 outFrag;

const float GOLDEN_ANGLE = 2.39996323;
const float MAX_BLUR_SIZE = 20.0;
const float RAD_SCALE = 1.0; // Smaller = nicer blur, larger = faster

float GetBlurSize(float depth, float focusPoint, float focusScale)
{
	float coc = clamp((1.0 / focusPoint - 1.0 / depth) * focusScale, -1.0, 1.0);
	return abs(coc) * MAX_BLUR_SIZE;
}

vec3 DepthOfField(vec2 texCoord, float focusPoint, float focusScale, vec2 texelSize)
{
	float centerDepth = LinearizeDepth(texture(u_DepthTexture, texCoord).r, ubo.DepthUnpackConsts.x , ubo.DepthUnpackConsts.y);
	float centerSize = GetBlurSize(centerDepth, focusPoint, focusScale);
	vec3 color = texture(u_Texture, texCoord).rgb;
	float tot = 1.0;
	float radius = RAD_SCALE;
	for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
	{
		vec2 tc = texCoord + vec2(cos(ang), sin(ang)) * texelSize * radius;
		vec3 sampleColor = texture(u_Texture, tc).rgb;
		float sampleDepth =  LinearizeDepth(texture(u_DepthTexture, tc).r, ubo.DepthUnpackConsts.x , ubo.DepthUnpackConsts.y);
		float sampleSize = GetBlurSize(sampleDepth, focusPoint, focusScale);
		if (sampleDepth > centerDepth)
			sampleSize = clamp(sampleSize, 0.0, centerSize * 2.0);
		float m = smoothstep(radius - 0.5, radius + 0.5, sampleSize);
		color += mix(color / tot, sampleColor, m);
		tot += 1.0;
		radius += RAD_SCALE / radius;
	}
	return color /= tot;
}

void main()
{
	ivec2 texSize = textureSize(u_Texture, 0);
	vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));

	float focusPoint = LinearizeDepth(texture(u_DepthTexture, vec2(0.5f, 0.5f)).r, ubo.DepthUnpackConsts.x , ubo.DepthUnpackConsts.y);
	float blurScale = ubo.DOFParams.y;

	vec3 color = DepthOfField(outTexCoord, focusPoint, blurScale, 1.0 / fTexSize);
	outFrag = vec4(color, 1.0);
}