#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UBO
{
    mat4 projView;
    vec4 camPos;       // xyz = world position, w = global sky-fade brightness mult
    vec4 screenParams; // x = width px, y = height px, z = tan(fov/2), w = min size px
    vec4 streak;       // xyz = world travel dir, w = streak amount 0..1 (warp)
} ubo;

// Flat vec4 layout (2 vec4 per point) - MoltenVK-safe.
layout(std430, set = 1, binding = 0) readonly buffer PointBuffer
{
    vec4 data[];
} u_Points;

const int POINT_VEC4S = 2;

layout(location = 0) in vec2 inCorner; // [-0.5, 0.5]
layout(location = 1) in vec2 inUV;     // [0, 1]

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColour;

void main()
{
    int base     = gl_InstanceIndex * POINT_VEC4S;
    vec4 posSize = u_Points.data[base + 0]; // xyz = world pos, w = world size
    vec4 colour  = u_Points.data[base + 1];

    vec3 center   = posSize.xyz;
    float distCam = length(center - ubo.camPos.xyz);

    vec4 clip = ubo.projView * vec4(center, 1.0);

    float halfPx = max(posSize.w, ubo.screenParams.w) * 0.5;

    vec2 corner = inCorner;
    float streakAmt = clamp(ubo.streak.w, 0.0, 1.0);
    float dim = 1.0;
    if(streakAmt > 0.001)
    {
        vec4 dirClip = ubo.projView * vec4(normalize(ubo.streak.xyz), 0.0);
        vec2 sd = dirClip.xy;
        float sdLen = length(sd);
        if(sdLen > 1e-4)
        {
            sd /= sdLen;
            vec3 toStar = normalize(center - ubo.camPos.xyz);
            float perp = 1.0 - abs(dot(toStar, normalize(ubo.streak.xyz)));
            float stretch = 1.0 + streakAmt * (2.0 + 26.0 * perp);
            float along = dot(corner, sd);
            vec2 perpC = corner - sd * along;
            corner = sd * (along * stretch) + perpC;
            // Conserve brightness-ish: long streaks render dimmer.
            dim = 1.0 / sqrt(stretch);
        }
    }

    vec2 px  = corner * (2.0 * halfPx);
    vec2 ndc = vec2(px.x / (ubo.screenParams.x * 0.5), px.y / (ubo.screenParams.y * 0.5));
    clip.xy += ndc * clip.w;
    gl_Position = clip;

    float ref   = 6.0; // reference distance (was passed in camPos.w, now the fade)
    float atten = clamp((ref * ref) / (distCam * distCam + 1e-4), 0.85, 1.5);
    float skyFade = clamp(ubo.camPos.w, 0.0, 1.0);

    vUV     = inUV;
    vColour = vec4(colour.rgb * colour.a * atten * skyFade * dim, 1.0);
}
