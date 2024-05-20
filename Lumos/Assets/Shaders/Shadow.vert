#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"

layout(push_constant) uniform PushConsts
{
	mat4 transform;
	uint cascadeIndex;
    float p0;
    float p1;
    float p2;
} pushConsts;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec2 uv;

void main()
{
    mat4 transform = pushConsts.transform;
    mat4 proj;
    switch(pushConsts.cascadeIndex)
    {
        case 0 : 
		proj = u_DirShadow.DirLightMatrices[0];
        break;
        case 1 : 
		proj = u_DirShadow.DirLightMatrices[1];
        break;
        case 2 : 
		proj = u_DirShadow.DirLightMatrices[2];
        break;
        default : 
		proj = u_DirShadow.DirLightMatrices[3];
        break;
    }
    //gl_Position = u_DirShadow.DirLightMatrices[pushConsts.cascadeIndex] * transform * vec4(inPosition, 1.0); 
	
	gl_Position = transform * vec4(inPosition, 1.0); 
    //This order needed to match the order of 
    /*

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;
*/
	vec4 test2 = inColor; //SPV vertex layout incorrect when not used
    uv = inTexCoord;
	vec3 test5 = inNormal; //SPV vertex layout incorrect when not used
	vec3 test3 = inTangent; //SPV vertex layout incorrect when not used
	vec3 test4 = inBitangent; //SPV vertex layout incorrect when not used
    float test6 = pushConsts.p0;
    float test7 = pushConsts.p1;
    float test8 = pushConsts.p2;
    
}