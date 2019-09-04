// Simple Texture Shader

#shader vertex
#version 430

layout(location = 0) in vec3 a_Position;
layout(location = 4) in vec2 a_TexCoord;

uniform mat4 u_MVP;

out vec2 v_TexCoord;

void main()
{
	vec4 position = u_MVP * vec4(a_Position, 1.0);
	gl_Position = position;

	v_TexCoord = a_TexCoord;
}

#shader fragment
#version 430

layout(location = 0) out vec4 color;

uniform float u_Scale;
uniform float u_Res;

in vec2 v_TexCoord;

float grid(vec2 st, float res)
{
	vec2 grid = fract(st);
	return step(res, grid.x) * step(res, grid.y);
}
 
void main()
{
	float scale = u_Scale;
	float resolution = u_Res;

	float x = grid(v_TexCoord * scale, resolution);
	color = vec4(vec3(0.2), 0.5) * (1.0 - x);
}