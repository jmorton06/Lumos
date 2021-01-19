#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec4 texColour = texture(texSampler, fragTexCoord);
	if(texColour.w < 0.4)
		discard;

    outColor = texColour; // * vec4(fragColor, 1.0f);
}