#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Graphics::PackedVertex (suffixes give the formats, see VKShader)
layout(location = 0) in vec3 inPosition_f32x3;
layout(location = 1) in vec4 inNormal_snorm16;
layout(location = 2) in vec4 inTangent_snorm16; // w = bitangent sign
layout(location = 3) in vec2 inTexCoord_f32x2;
layout(location = 4) in vec4 inColor_unorm8;

layout(set = 0,binding = 0) uniform UBO 
{    
	mat4 view;
	mat4 proj;
	mat4 u_MVP;
} ubo;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 nearPoint;
layout(location = 3) out vec3 farPoint; 
layout(location = 4) out mat4 fragView;
layout(location = 8) out mat4 fragProj;

out gl_PerVertex
{
    vec4 gl_Position;
};

vec3 gridPlane[6] = vec3[](
						   vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
						   vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
						   );

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint = viewInv *  projInv * vec4(x, y, z, 1.0) ;
    return unprojectedPoint.xyz / unprojectedPoint.w;
}


void main()
{
	vec4 position = vec4(inPosition_f32x3, 1.0);// * ubo.u_MVP;
    gl_Position  = position;
	fragPosition = inPosition_f32x3;
	
	vec4 test = inColor_unorm8; //SPV vertex layout incorrect when inColor_unorm8 not used
	fragTexCoord = inTexCoord_f32x2;
	
	fragView = ubo.view;
	fragProj = ubo.proj;
	
	vec3 p =  position.xyz;//gridPlane[gl_VertexIndex].xyz;
    // reversed-Z: depth 1 is the near plane, 0 the far one
    nearPoint = UnprojectPoint(p.x, p.y, 1.0, ubo.view, ubo.proj).xyz;
    farPoint = UnprojectPoint(p.x, p.y, 0.0, ubo.view, ubo.proj).xyz;
    gl_Position = vec4(p.xyz, 1.0); // using directly
}