#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out float out_Colour;
layout (location = 0) in vec2 ex_TexCoord;

layout (set = 0, binding = 0) uniform UniformBuffer
{
    vec2 ssaoTexelOffset;
	int ssaoBlurRadius;
	int pad;
} ubo;

layout (set = 0,binding = 1) uniform sampler2D in_Depth;
layout (set = 0,binding = 2) uniform sampler2D in_SSAO;
layout (set = 0,binding = 3) uniform sampler2D in_Normal;

void main()
{
	float ourDepth = texture(in_Depth, ex_TexCoord).r;
	vec3 ourNormal = normalize(texture(in_Normal, ex_TexCoord).rgb * 2.0f - 1.0f);

	int sampleCount = 0;
	float sum = 0.0f;
	for (int i = -ubo.ssaoBlurRadius; i <= ubo.ssaoBlurRadius; i++) 
	{
		vec2 offset = ubo.ssaoTexelOffset * float(i);
		float depth = texture(in_Depth, ex_TexCoord + offset).r;
		vec3 normal = normalize(texture(in_Normal, ex_TexCoord + offset).rgb * 2.0f - 1.0f);
		if (abs(ourDepth - depth) < 0.00002f && dot(ourNormal, normal) > 0.85f)
		{
			sum += texture(in_SSAO, ex_TexCoord + offset).r;
			++sampleCount;
		}
	}
	out_Colour = clamp(sum / float(sampleCount), 0.0f, 1.0f);
}