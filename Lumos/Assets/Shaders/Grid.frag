#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4 u_CameraPos;
	float u_Scale;
	float u_Res;
	float u_MaxDistance;
	float p1;
}
ubo;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;

const float step = 100.0f;
const float subdivisions = 10.0f;

vec4 Grid(float divisions)
{
	vec2 coord = fragTexCoord.xy * divisions;

	vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
	float line = min(grid.x, grid.y);
	float lineResult = ubo.u_Res - min(line, ubo.u_Res);
	vec3 colour = vec3(0.3, 0.3, 0.3);

	return vec4(vec3(lineResult) * colour, 0.1 * lineResult);
}

void main()
{
	vec3 pseudoViewPos = vec3(ubo.u_CameraPos.x, fragPosition.y, ubo.u_CameraPos.z);
	float distanceToCamera = max(distance(fragPosition, pseudoViewPos) - abs(ubo.u_CameraPos.y), 0);

	float divs = ubo.u_Scale / pow(2, round((abs(ubo.u_CameraPos.y) - step / subdivisions) / step));

	float decreaseDistance = ubo.u_MaxDistance * 1.5;

	outColour = Grid(divs) + Grid(divs / subdivisions);
	outColour.a *= clamp((decreaseDistance - distanceToCamera) / decreaseDistance, 0.0f, 1.0f);
}