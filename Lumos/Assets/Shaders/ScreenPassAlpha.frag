#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;

// Transparent windows: premultiplied colour + coverage straight to the swapchain
void main()
{
	vec4 colour = texture(u_Texture, outTexCoord);
	outFrag = vec4(min(colour.rgb, vec3(colour.a)), colour.a);
}
