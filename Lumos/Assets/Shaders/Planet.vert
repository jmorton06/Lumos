#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UBO
{
    mat4 projView;
    vec4 camPos;       // xyz = camera world pos, w = time (seconds)
    vec4 camRight;     // xyz = camera right
    vec4 camUp;        // xyz = camera up
    vec4 screenParams; // x = width px, y = height px, z = tan(fov/2), w = min planet px
} ubo;

// Flat vec4 layout (4 vec4 per planet) - MoltenVK-safe.
layout(std430, set = 1, binding = 0) readonly buffer PlanetBuffer
{
    vec4 data[];
} u_Planets;

const int PLANET_VEC4S = 4;

layout(location = 0) in vec2 inCorner; // [-1, 1]

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) flat out vec4 vPosRadius; // xyz centre, w radius
layout(location = 2) flat out vec4 vStar;      // xyz star pos, w type
layout(location = 3) flat out vec4 vTint;      // rgb base colour, w seed
layout(location = 4) flat out vec4 vParams;    // x tilt, y cloud, z spin, w rings
layout(location = 5) flat out vec2 vQuad;      // x = quad half size (world), y = upscale factor (1 = true size)

void main()
{
    int base = gl_InstanceIndex * PLANET_VEC4S;
    vec4 posRadius = u_Planets.data[base + 0];

    vec3 center  = posRadius.xyz;
    float radius = posRadius.w;

    vec4 params  = u_Planets.data[base + 3];
    bool hasRing = params.w > 0.5;
    bool isStar  = int(u_Planets.data[base + 1].w + 0.5) == 3;
    float quadR  = radius * (isStar ? 4.5 : (hasRing ? 3.4 : 1.5));

    float dist   = max(length(center - ubo.camPos.xyz), 1e-6);
    float sizePx = quadR * ubo.screenParams.y / (dist * ubo.screenParams.z);
    float minPx  = ubo.screenParams.w;
    float upscale = (sizePx < minPx && sizePx > 0.0) ? (minPx / sizePx) : 1.0;
    quadR *= upscale;

    vec3 worldPos = center + ubo.camRight.xyz * (inCorner.x * quadR) + ubo.camUp.xyz * (inCorner.y * quadR);

    vWorldPos  = worldPos;
    vPosRadius = posRadius;
    vStar      = u_Planets.data[base + 1];
    vTint      = u_Planets.data[base + 2];
    vParams    = u_Planets.data[base + 3];
    vQuad      = vec2(quadR, upscale);

    gl_Position = ubo.projView * vec4(worldPos, 1.0);
}
