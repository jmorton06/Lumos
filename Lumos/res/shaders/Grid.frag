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

float grid(vec2 st, float res)
{
	vec2 grid = fract(st);
	return step(res, grid.x) * step(res, grid.y);
}

const float step = 100.0f;
const float subdivisions = 4.0f;

vec4 Grid(float p_divisions)
{
    vec2 coord = v_TexCoord.xy * p_divisions;

    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    float lineResult = ubo.u_Res - min(line, ubo.u_Res);
	
    return vec4(vec3(lineResult) * vec3(0.8,0.8,0.8), 0.1 * lineResult);
}

void main()
{
	vec3 pseudoViewPos = vec3(ubo.u_CameraPos.x, v_Position.y, ubo.u_CameraPos.z);
    float distanceToCamera = max(distance(v_Position, pseudoViewPos) - abs(ubo.u_CameraPos.y), 0);

	float divs;
	divs = ubo.u_Scale / pow(2, round((abs(ubo.u_CameraPos.y) - step / subdivisions) / step));

	float distanceAlpha = 1.0f;
	float alphaDecreaseDistance = ubo.u_MaxDistance;
    float decreaseDistance = ubo.u_MaxDistance * 1.5;

	distanceAlpha = (decreaseDistance - distanceToCamera) / decreaseDistance;
	distanceAlpha = clamp(distanceAlpha , 0.0f, 1.0f);
	if (distanceToCamera > alphaDecreaseDistance)
    {
        //float normalizedDistanceToCamera = clamp(distanceToCamera - alphaDecreaseDistance, 0.0f, decreaseDistance) / decreaseDistance;
       	//distanceAlpha = clamp(1.0f - normalizedDistanceToCamera, 0.0f, 1.0f);

		
		outColour = Grid(divs / subdivisions);
    }
	else
	{
		outColour = Grid(divs) + Grid(divs / subdivisions);
	}
  
  	outColour.a *= distanceAlpha;

	if(outColour.a < 0.1)
		discard;
}