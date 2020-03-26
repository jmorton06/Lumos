#shader vertex
layout(location = 0) in vec3 a_Position;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 sys_InvViewProjMatrix;


out vec2 v_TexCoord;
out vec3 v_Position;

void main()
{
	vec4 position = vec4(a_Position, 1.0) * sys_InvViewProjMatrix;
	gl_Position = position;
	v_Position = a_Position.xyz;

	v_TexCoord = a_TexCoord;
}

#shader end

#shader fragment
layout(location = 0) out vec4 outColour;

layout(std140) uniform UniformBuffer
{
	vec4 u_CameraPos;
	float u_Scale;
	float u_Res;
	float u_MaxDistance;
	float p1;
} ubo;

in vec2 v_TexCoord;
in vec3 v_Position;
 
const float step = 100.0f;
const float subdivisions = 4.0f;

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
#shader end