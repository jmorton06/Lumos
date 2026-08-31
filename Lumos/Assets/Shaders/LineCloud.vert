#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UBO
{
    mat4 projView;
    vec4 camPos;       // xyz world position
    vec4 screenParams; // x = width px, y = height px, z = tan(fov/2), w = min width px
} ubo;

// 3 vec4 per segment (flat layout, MoltenVK-safe): A(xyz)+width, B(xyz), colour.
layout(std430, set = 1, binding = 0) readonly buffer LineBuffer
{
    vec4 data[];
} u_Lines;

const int LINE_VEC4S = 4;

layout(location = 0) in vec2 inCorner; // x = end (0=A,1=B), y = side (-1..+1)

layout(location = 0) out vec4 vColour;
layout(location = 1) out float vSide;
layout(location = 2) out float vAlong;    // arc length along the path at this vertex
layout(location = 3) out vec2 vDash;      // x = dash period, y = duty

void main()
{
    int base     = gl_InstanceIndex * LINE_VEC4S;
    vec3 A       = u_Lines.data[base + 0].xyz;
    float width  = u_Lines.data[base + 0].w;
    vec3 B       = u_Lines.data[base + 1].xyz;
    float arcAtA = u_Lines.data[base + 1].w;
    vec4 colour  = u_Lines.data[base + 2];
    vec2 dash    = u_Lines.data[base + 3].xy;

    bool atB = inCorner.x > 0.5;
    vec3 pos = atB ? B : A;

    float widthB = u_Lines.data[base + 3].z;
    if(atB && widthB != 0.0)
        width = widthB;

    // Camera-facing ribbon: offset perpendicular to the line, facing the camera.
    vec3 lineDir = normalize(B - A);
    vec3 viewDir = normalize(pos - ubo.camPos.xyz);
    vec3 perp    = normalize(cross(lineDir, viewDir));

    // Width. The two conventions are kept strictly apart:
    //   width > 0 -> a pixel width, converted here using the distance to the eye
    //   width < 0 -> ALREADY a world width (the caller did its own px maths)
    //
    // The world path must NOT be routed through the eye. Callers draw in three
    // different frames - world, the star-local AU frame, and CAMERA-RELATIVE
    // (ships, stations, markers, where positions are p - cam and the effective
    // eye is the origin) - so `length(pos - camPos)` is only meaningful for
    // some of them. Sizing a world width by it inflates the quad enormously in
    // the camera-relative case. The fragment stage gets its pixel scale from
    // screen-space derivatives instead, which is correct in every frame.
    float halfWorld;
    if(width < 0.0)
    {
        halfWorld = (-width) * 0.5;
    }
    else
    {
        float distCam    = length(pos - ubo.camPos.xyz);
        float pxPerWorld = (ubo.screenParams.y * 0.5) / max(ubo.screenParams.z * distCam, 1e-4);
        float halfPx     = max(width, ubo.screenParams.w) * 0.5;
        halfWorld        = halfPx / pxPerWorld;
    }

    vec3 world = pos + perp * (inCorner.y * halfWorld);

    gl_Position = ubo.projView * vec4(world, 1.0);
    vColour     = colour;
    vSide       = inCorner.y;
    vAlong      = arcAtA + (atB ? length(B - A) : 0.0);
    vDash       = dash;
}
