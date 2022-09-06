#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform PushConsts
{
	mat4 transform;
	uint cascadeIndex;
} pushConsts;

layout(set = 0,binding = 0) uniform UBO
{
    mat4 projView[16];
} ubo;

const int MAX_BONES = 100;

layout(set = 0,binding = 1) uniform UBOAnim
{    
	mat4 BoneTransforms[MAX_BONES];
} boneUbo;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout (location = 0) in vec3 position;

layout(location = 5) in ivec4 inBoneIndices;
layout(location = 6) in vec4 inBoneWeights;

void main()
{
    mat4 proj;
    switch(pushConsts.cascadeIndex)
    {
        case 0 : 
            proj = ubo.projView[0];
            break;
        case 1 : 
            proj = ubo.projView[1];
            break;
        case 2 : 
            proj = ubo.projView[2];
            break;
        default : 
            proj = ubo.projView[3];
            break;
    }

    mat4 boneTransform = boneUbo.BoneTransforms[int(inBoneIndices[0])] * inBoneWeights[0];
    boneTransform += boneUbo.BoneTransforms[int(inBoneIndices[1])] * inBoneWeights[1];
    boneTransform += boneUbo.BoneTransforms[int(inBoneIndices[2])] * inBoneWeights[2];
    boneTransform += boneUbo.BoneTransforms[int(inBoneIndices[3])] * inBoneWeights[3];

    gl_Position = vec4(position, 1.0) * pushConsts.transform * boneTransform * proj; 
}