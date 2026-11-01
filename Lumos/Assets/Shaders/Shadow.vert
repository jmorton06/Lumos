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

// Graphics::PackedVertex (suffixes give the formats, see VKShader)
layout(location = 0) in vec3 inPosition_f32x3;
layout(location = 1) in vec4 inNormal_snorm16;
layout(location = 2) in vec4 inTangent_snorm16; // w = bitangent sign
layout(location = 3) in vec2 inTexCoord_f32x2;
layout(location = 4) in vec4 inColor_unorm8;

layout(location = 0) out vec2 uv;

void main()
{
    mat4 transform = pushConsts.transform;
    gl_Position = transform * vec4(inPosition_f32x3, 1.0);

	vec4 test2 = inColor_unorm8; //SPV vertex layout incorrect when not used
    uv = inTexCoord_f32x2;
	vec3 test5 = inNormal_snorm16.xyz; //SPV vertex layout incorrect when not used
	vec3 test3 = inTangent_snorm16.xyz; //SPV vertex layout incorrect when not used
}
