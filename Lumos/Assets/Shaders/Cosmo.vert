#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UBO
{
	mat4 invProjection;
	mat4 invView;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec3 outPosition;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
	vec4 pos = vec4(inPosition.xy, 1.0 ,1.0);
	gl_Position = pos;

	outPosition = mat3(ubo.invView) * vec3(ubo.invProjection * pos);
}
