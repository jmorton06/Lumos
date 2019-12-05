#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 outColour;

layout(set = 0,binding = 1) uniform UniformBuffer
{
	vec4 u_CameraPos;
	float u_Scale;
	float u_Res;
	float u_MaxDistance;
	float p1;
} ubo;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_Position;

const float step = 100.0f;
const float subdivisions = 10.0f;

vec4 Grid(float p_divisions)
{
    vec2 coord = v_TexCoord.xy * p_divisions;

    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    float lineResult = ubo.u_Res - min(line, ubo.u_Res);
	vec3 colour = vec3(0.2,0.2,0.2);

	if(v_TexCoord.x < 0.5001 && v_TexCoord.x > 0.4999)
	{
		colour = vec3(1.0f, 0.0f, 0.0f);
		lineResult *= 2.0f;
	}
	else if(v_TexCoord.y < 0.5001 && v_TexCoord.y > 0.4999)
	{
		colour = vec3(0.0f, 0.0f, 1.0f);
		lineResult *= 2.0f;
	}
    return vec4(vec3(lineResult) * colour, 0.1 * lineResult);
}

void main()
{
	vec3 pseudoViewPos = vec3(ubo.u_CameraPos.x, v_Position.y, ubo.u_CameraPos.z);
    float distanceToCamera = max(distance(v_Position, pseudoViewPos) - abs(ubo.u_CameraPos.y), 0);

	float divs = ubo.u_Scale / pow(2, round((abs(ubo.u_CameraPos.y) - step / subdivisions) / step));

	float alphaDecreaseDistance = ubo.u_MaxDistance;
    float decreaseDistance = ubo.u_MaxDistance * 1.5;

	outColour = Grid(divs) + Grid(divs / subdivisions);
  	outColour.a *= clamp((decreaseDistance - distanceToCamera) / decreaseDistance , 0.0f, 1.0f);
}