#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UniformBufferObject
{
	mat4 invprojview;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outPosition;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
		vec4 pos = vec4(inPosition,1.0);
		pos.z = 1.0f;
		gl_Position = pos;
		outPosition = pos * ubo.invprojview;
}
