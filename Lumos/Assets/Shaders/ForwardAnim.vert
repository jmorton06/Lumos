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

const int MAX_BONES = 100;
layout(set = 0,binding = 1) uniform UBOAnim
{    
	mat4 BoneTransforms[MAX_BONES];
} boneUbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 6) in ivec4 inBoneIndices;
layout(location = 7) in vec4 inBoneWeights;

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) out VertexData VertexOutput;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	VertexOutput.Position = pushConsts.transform * vec4(inPosition, 1.0);
	
    mat4 boneTransform = boneUbo.BoneTransforms[int(inBoneIndices[0])] * inBoneWeights[0];
    boneTransform += boneUbo.BoneTransforms[int(inBoneIndices[1])] * inBoneWeights[1];
    boneTransform += boneUbo.BoneTransforms[int(inBoneIndices[2])] * inBoneWeights[2];
    boneTransform += boneUbo.BoneTransforms[int(inBoneIndices[3])] * inBoneWeights[3];
	
    gl_Position = cameraUBO.projView * boneTransform *  VertexOutput.Position;
    
	VertexOutput.Colour = inColor.xyz;
	VertexOutput.TexCoord = inTexCoord;
	//VertexOutput.Normal = mat3(pushConsts.transform) * inNormal;
	mat3 transposeInv = transpose(inverse(mat3(pushConsts.transform) * mat3(boneTransform) ));
    VertexOutput.Normal = transposeInv * inNormal;
    
    VertexOutput.WorldNormal = transposeInv * mat3(inTangent, inBitangent, inNormal);
    
}