#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "Common.glslh"

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

//Based on https://gist.github.com/bgolus/d49651f52b1dcf82f70421ba922ed064
float PristineGrid(vec2 uv, vec2 lineWidth) 
{
    vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
    bvec2 invertLine = lessThan(lineWidth, vec2(0.5));
    vec2 targetWidth = mix(1.0 - lineWidth, lineWidth, invertLine);
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV = mix(gridUV, 1.0 - gridUV, invertLine);
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(targetWidth / drawWidth);
    grid2 = mix(grid2, targetWidth, saturate(uvDeriv * 2.0 - 1.0));
    grid2 = mix(1.0 - grid2, grid2, invertLine);
    return mix(grid2.x, 1.0, grid2.y);
}

 vec4 PristineGrid2(vec4 uv, vec2 lineWidth) 
{
    vec4 uvDDXY = vec4(dFdx(uv.xy), dFdy(uv.xy));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
    
    float axisLineWidth = max(lineWidth.x, 0.08);
    vec2 axisDrawWidth = max(vec2(axisLineWidth, axisLineWidth), uvDeriv);
    vec2 axisLineAA = uvDeriv * 1.5;
    vec2 axisLines2 = smoothstep(axisDrawWidth + axisLineAA, axisDrawWidth - axisLineAA, abs(uv.zw * 2.0));
    axisLines2 *= saturate(axisLineWidth / axisDrawWidth);
    
    float div = max(2.0, round(lineWidth.x));
    vec2 majorUVDeriv = uvDeriv / div;
    float majorLineWidth = lineWidth.x / div;
    vec2 majorDrawWidth = vec2(clamp(majorLineWidth, majorUVDeriv.x, 0.5));
    vec2 majorLineAA = majorUVDeriv * 1.5;
    vec2 majorGridUV = abs(fract(uv.xy / div) * 2.0 - 1.0) ;
    majorGridUV = vec2(1.0 - majorGridUV.x, 1.0 - majorGridUV.y);
    vec2 majorAxisOffset = (1.0 - saturate(abs(uv.xy / div * 2.0))) * 2.0;
    majorGridUV += majorAxisOffset; // adjust UVs so center axis line is skipped
    vec2 majorGrid2 = smoothstep(majorDrawWidth + majorLineAA, majorDrawWidth - majorLineAA, majorGridUV);
    majorGrid2 *= saturate(majorLineWidth / majorDrawWidth);
    majorGrid2 = saturate(majorGrid2 - axisLines2); // hack
    majorGrid2 = mix(majorGrid2, vec2(majorLineWidth), saturate(majorUVDeriv * 2.0 - vec2(1.0)));
    
    float minorLineWidth = min(lineWidth.x / 2.0, lineWidth.x);
    bool minorInvertLine = minorLineWidth > 0.5;
    float minorTargetWidth = minorInvertLine ? 1.0 - minorLineWidth : minorLineWidth;
    vec2 minorDrawWidth = clamp(vec2(minorTargetWidth, 0.0), uvDeriv, vec2(0.5, 0.5));
    vec2 minorLineAA = uvDeriv * 1.5;
    vec2 minorGridUV = abs(fract(uv.xy) * 2.0 - 1.0);
    minorGridUV = minorInvertLine ? minorGridUV : 1.0 - minorGridUV;
    vec2 minorMajorOffset = (1.0 - saturate((1.0 - abs(fract(uv.zw / div) * 2.0 - 1.0)) * div)) * 2.0;
    minorGridUV += minorMajorOffset; // adjust UVs so major division lines are skipped
    vec2 minorGrid2 = smoothstep(minorDrawWidth + minorLineAA, minorDrawWidth - minorLineAA, minorGridUV);
    minorGrid2 *= saturate(minorTargetWidth / minorDrawWidth);
    minorGrid2 = saturate(minorGrid2 - axisLines2); // hack
    minorGrid2 = mix(minorGrid2, vec2(minorTargetWidth), saturate(uvDeriv * 2.0 - 1.0));
    minorGrid2 = minorInvertLine ? 1.0 - minorGrid2 : minorGrid2;
    minorGrid2 = abs(uv.zw).x > 0.5 ? minorGrid2 : vec2(0.0);
    
    float minorGrid = mix(minorGrid2.x, 1.0, minorGrid2.y);
    float majorGrid = mix(majorGrid2.x, 1.0, majorGrid2.y);
    
    vec2 axisDashUV = abs(fract((uv.zw + axisLineWidth * 0.5) * 1.3) * 2.0 - 1.0) - 0.5;
    vec2 axisDashDeriv = uvDeriv * 1.3 * 1.5;
    vec2 axisDash = smoothstep(-axisDashDeriv, axisDashDeriv, axisDashUV);
    axisDash = uv.z < 0.0 ? axisDash : vec2(1.0);
    
    vec3 aAxisColor = mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0), axisDash.y);
    vec3 bAxisColor = mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0), axisDash.x);
	vec3 centerColor = vec3(1.0, 0.0, 0.0);
    aAxisColor = mix(aAxisColor, centerColor, axisLines2.y);
    
    vec4 axisLines = vec4(mix(bAxisColor * axisLines2.y, aAxisColor, axisLines2.x), 1.0);
    vec4 minorLineColor = vec4(1.0, 0.0, 0.0, 0.5);
	vec4 majorLineColor = vec4(1.0, 1.0, 0.0, 0.5);
	vec4 baseColor = vec4(1.0, 0.0, 0.0, 0.5);
    vec4 col = mix(baseColor, minorLineColor, minorGrid *  minorLineColor.a);
    col = mix(col, majorLineColor, majorGrid * majorLineColor.a);
    col = col * (1.0 - axisLines.a) + axisLines;
    return col;
}
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
	float secondFade = smoothstep(1 / divs, 1 / divs * 10, distanceToCamera);
	
    //vec4 grid1 = grid(fragPos3D, divs, true);
	//vec4 grid2 = grid(fragPos3D, divs, true);
    
    
	//TODO: Fade second grid in distance
	//secondFade*= smoothstep(ubo.u_Far / 0.5, ubo.u_Far, distanceToCamera);
	//grid2.a *= secondFade;
	//grid1.a *= (1 - secondFade);
	
    //outColour =  grid1 + grid2;  // adding multiple resolution for the grid
	//outColour.a = max(grid1.a, grid2.a);
	
	vec2 uv = fragPos3D.xz * divs;
	float grid1 = PristineGrid(uv, vec2(0.01) * (1 - secondFade));
	uv = fragPos3D.xz * (divs / 10.0);
	float grid2 = PristineGrid(uv, vec2(0.01) * secondFade);
	
	grid2 *= secondFade;
	grid1 *= (1 - secondFade);
	
	outColour = vec4(vec3(0.45, 0.45, 0.45), grid1 + grid2);
	outColour.a = max(grid1, grid2);
    outColour *= float(t > 0);
	//outColour.a *= fading;
	outColour.a *= angleFade;
    
    //vec2 uv = fragPos3D.xz * divs;
    //vec2 uv = fragPos3D.xz;// / fragPos3D.w;
    //float grid = PristineGrid(uv, vec2(0.005));
    //outColour = vec4(vec3(0.0), grid);
	//vec4 test = vec4(fragPos3D, 1.0);
	//outColour = PristineGrid2(test, vec2(0.005));
}