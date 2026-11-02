#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UBO
{
	mat4 invProjection;
	mat4 invView;
} ubo;

// Graphics::PackedVertex (suffixes give the formats, see VKShader)
layout(location = 0) in vec3 inPosition_f32x3;
layout(location = 1) in vec4 inNormal_snorm16;
layout(location = 2) in vec4 inTangent_snorm16; // w = bitangent sign
layout(location = 3) in vec2 inTexCoord_f32x2;
layout(location = 4) in vec4 inColor_unorm8;

layout(location = 0) out vec3 outPosition;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
	vec4 pos = vec4(inPosition_f32x3.xy, 0.0, 1.0); // far plane (reversed-Z)
	gl_Position = pos;

	outPosition = mat3(ubo.invView) * vec3(ubo.invProjection * pos);
}
