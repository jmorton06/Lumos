#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UBO 
{    
	mat4 projView;
} cameraUBO;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

struct VertexOutput
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	vec3 Tangent;
	vec4 ShadowMapCoords[4];
};

layout(location = 0) out VertexOutput Output;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	Output.Position = pushConsts.transform * vec4(inPosition, 1.0);
    gl_Position = cameraUBO.projView * Output.Position;
    
	Output.Colour = inColor.xyz;
	Output.TexCoord = inTexCoord;
    Output.Normal = transpose(inverse(mat3(pushConsts.transform))) * normalize(inNormal);
    Output.Tangent = inTangent;
}