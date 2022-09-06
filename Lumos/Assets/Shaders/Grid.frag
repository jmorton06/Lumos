#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4 u_CameraPos;
	vec4 u_CameraForward;
	float u_Near;
	float u_Far;
	float u_MaxDistance;
	float p1;
}
ubo;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 2) in vec3 nearPoint;
layout(location = 3) in vec3 farPoint;
layout(location = 4) in mat4 fragView;
layout(location = 8) in mat4 fragProj;

const float step = 100.0f;
const float subdivisions = 10.0f;


vec4 grid(vec3 fragPos3D, float scale, bool drawAxis) 
{
    vec2 coord = fragPos3D.xz * scale;
	
	vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 colour = vec4(0.35, 0.35, 0.35, 1.0 - min(line, 1.0));
    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        colour = vec4(0.0, 0.0, 1.0, colour.w);
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        colour = vec4(1.0, 0.0, 0.0, colour.w);
    return colour;
}
float computeDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}
float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos =  fragProj * fragView * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * ubo.u_Near * ubo.u_Far) / (ubo.u_Far + ubo.u_Near - clip_space_depth * (ubo.u_Far - ubo.u_Near)); // get linear value between 0.01 and 100
    return linearDepth / ubo.u_Far; // normalize
}

int roundToPowerOfTen(float n)
{
    return int(pow(10.0, floor( (1 / log(10)) * log(n))));
}

void main()
{
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
	if (t < 0.)
        discard;
	
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);
	
    gl_FragDepth = computeDepth(fragPos3D);
	
    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0, (0.5 - linearDepth));
	
	float decreaseDistance = ubo.u_Far * 1.5;
	vec3 pseudoViewPos = vec3(ubo.u_CameraPos.x, fragPos3D.y, ubo.u_CameraPos.z);
	
	  float dist, angleFade;
  /* if persp */
  if (fragProj[3][3] == 0.0) {
    vec3 viewvec = ubo.u_CameraPos.xyz - fragPos3D;
    dist = length(viewvec);
    viewvec /= dist;

    float angle;
      angle = viewvec.y;
    angle = 1.0 - abs(angle);
    angle *= angle;
		angleFade= 1.0 - angle * angle;
		angleFade*= 1.0 - smoothstep(0.0, ubo.u_MaxDistance, dist - ubo.u_MaxDistance);
  }
  else {
    dist = gl_FragCoord.z * 2.0 - 1.0;
    /* Avoid fading in +Z direction in camera view (see T70193). */
    dist = clamp(dist, 0.0, 1.0);// abs(dist);
		angleFade = 1.0 - smoothstep(0.0, 0.5, dist - 0.5);
    dist = 1.0; /* avoid branch after */

  }
	
	float distanceToCamera = abs(ubo.u_CameraPos.y - fragPos3D.y);
	int powerOfTen = roundToPowerOfTen(distanceToCamera);
	powerOfTen = max(1, powerOfTen);
	float divs = 1.0 / float(powerOfTen);
	float secondFade = smoothstep(subdivisions / divs, 1 / divs, distanceToCamera);
	
	vec4 grid1 = grid(fragPos3D, divs/subdivisions, true);
	vec4 grid2 = grid(fragPos3D, divs, true);
	
	//TODO: Fade second grid in distance
	//secondFade*= smoothstep(ubo.u_Far / 0.5, ubo.u_Far, distanceToCamera);
	grid2.a *= secondFade;
	grid1.a *= (1 - secondFade);
	
    outColour =  grid1 + grid2;  // adding multiple resolution for the grid
	outColour.a = max(grid1.a, grid2.a);
	
		outColour *= float(t > 0);
	outColour.a *= fading;
	outColour.a *= angleFade;
}