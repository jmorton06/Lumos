#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"

layout(push_constant) uniform PushConsts
{
	mat4 transform;
	uint cascadeIndex;
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

    
}