#shader vertex

layout(location = 0) in vec3 position;

uniform mat4 sys_InvViewProjMatrix;

out Vertex
{
    vec4 position;
} OUT;

void main (void)
{
	vec4 pos = vec4(position,1.0);
	pos.z = 1.0f;
	gl_Position = pos;
	OUT.position = sys_InvViewProjMatrix * pos;
}

#shader fragment

in Vertex
{
	vec4 position;
} IN;

uniform samplerCube u_CubeMap;

out vec4 outFrag;

void main()
{
	outFrag = texture(u_CubeMap, IN.position.xyz);
}
