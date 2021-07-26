#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UniformBufferObject 
{    
	mat4 projView;
} ubo;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragTangent;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	fragPosition = vec4(inPosition, 1.0) * pushConsts.transform;
    gl_Position = fragPosition * ubo.projView;
    
	fragColor = inColor.xyz;
	fragTexCoord = inTexCoord;
    fragNormal = normalize(inNormal) * transpose(inverse(mat3(pushConsts.transform)));
    fragTangent = inTangent;
}