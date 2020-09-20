#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 a_Position;
layout(location = 2) in vec2 a_TexCoord;

layout(set = 0,binding = 0) uniform UniformBufferObject 
{    
	mat4 u_MVP;
} ubo;

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_Position;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
	vec4 position = vec4(a_Position, 1.0) * ubo.u_MVP;
	gl_Position = position;
	v_Position = a_Position.xyz;

	v_TexCoord = a_TexCoord;
}