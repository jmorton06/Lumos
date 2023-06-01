#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;


void main()
{
	float amount = 0.7;
	
	vec4 colourTex = texture(u_Texture, outTexCoord);
	vec2 texSize   = textureSize(u_Texture, 0).xy;
	
	float neighbour = amount * -1.0;
	float centre   = amount *  4.0 + 1.0;
	
	vec3 colour =
        texture(u_Texture, outTexCoord + vec2( 0,  1) / texSize).rgb
		* neighbour
		+ texture(u_Texture, outTexCoord + vec2(-1,  0) / texSize).rgb
		* neighbour
		+ colourTex.rgb
		* centre
		+ texture(u_Texture, outTexCoord + vec2( 1,  0) / texSize).rgb
		* neighbour
		+ texture(u_Texture, outTexCoord + vec2( 0, -1) / texSize).rgb
		* neighbour;
	
	
	outFrag = vec4(colour, colourTex.a);
}