#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColour;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

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
	vec4 position = vec4(inPosition, 1.0);// * ubo.u_MVP;
    gl_Position  = position;
	fragPosition = inPosition;
	
	vec4 test = inColour; //SPV vertex layout incorrect when inColour not used
	fragTexCoord = inTexCoord;
	
	fragView = ubo.view;
	fragProj = ubo.proj;
	
	vec3 p =  position.xyz;//gridPlane[gl_VertexIndex].xyz;
    nearPoint = UnprojectPoint(p.x, p.y, 0.0, ubo.view, ubo.proj).xyz; // unprojecting on the near plane
    farPoint = UnprojectPoint(p.x, p.y, 1.0, ubo.view, ubo.proj).xyz; // unprojecting on the far plane
    gl_Position = vec4(p.xyz, 1.0); // using directly
}