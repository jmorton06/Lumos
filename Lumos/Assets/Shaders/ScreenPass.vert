#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec2 outTexCoord;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
	vec2 position = vec2(gl_VertexIndex % 2, gl_VertexIndex / 2) * 4.0 - 1 ;
	outTexCoord = (position + 1) * 0.5;
	gl_Position = vec4(position, 1.0, 1.0);
}
