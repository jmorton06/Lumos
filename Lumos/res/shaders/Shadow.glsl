#shader vertex

uniform mat4 sys_ProjectionViewMatrix;
uniform mat4 modelMatrix;

layout (location = 0) in vec3 position;

void main(void)
{
	gl_Position = sys_ProjectionViewMatrix * modelMatrix * vec4(position, 1.0);
}

#shader fragment

layout(location = 0) out float depth;

void main(void)
{
	depth = gl_FragCoord.z;
}
