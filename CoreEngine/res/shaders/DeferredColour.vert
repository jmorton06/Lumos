#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UniformBufferObject 
{    
	mat4 projView;
} ubo;

layout(set = 0,binding = 1) uniform UniformBufferObject2 
{
    mat4 model;
} ubo2;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
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
    gl_Position = ubo.projView * ubo2.model * vec4(inPosition, 1.0);
    fragPosition = ubo2.model * vec4(inPosition, 1.0);
    fragColor = inColor;
	fragTexCoord = inTexCoord;
    fragNormal = transpose(inverse(mat3(ubo2.model))) * normalize(inNormal);
    fragTangent = inTangent;
}