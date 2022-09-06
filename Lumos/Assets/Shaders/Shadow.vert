#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform PushConsts
{
	mat4 transform;
	uint cascadeIndex;
} pushConsts;

layout(set = 0,binding = 0) uniform ShadowData
{
    mat4 LightMatrices[16];
} ubo;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec2 uv;

void main()
{
    mat4 proj;
    switch(pushConsts.cascadeIndex)
    {
        case 0 : 
		proj = ubo.LightMatrices[0];
            break;
        case 1 : 
		proj = ubo.LightMatrices[1];
            break;
        case 2 : 
		proj = ubo.LightMatrices[2];
            break;
        default : 
		proj = ubo.LightMatrices[3];
            break;
    }
    gl_Position = proj * pushConsts.transform * vec4(inPosition, 1.0); 
	
	vec3 test = inPosition; //SPV vertex layout incorrect when not used
	vec4 test2 = inColor; //SPV vertex layout incorrect when not used
	uv = inTexCoord;
}