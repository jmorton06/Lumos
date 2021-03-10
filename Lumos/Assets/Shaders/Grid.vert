#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

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
	vec4 position = vec4(inPosition, 1.0) * ubo.u_MVP;
	gl_Position = position;
	v_Position = inPosition.xyz;

	v_TexCoord = inTexCoord;
}