#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UBO
{
    mat4 projView;
    vec4 camPos;       // xyz = world position, w = global sky-fade brightness mult
    vec4 screenParams; // x = width px, y = height px, z = tan(fov/2), w = sigma floor px
    vec4 streak;       // xyz = world travel dir, w = streak amount 0..1 (warp)
    vec4 look;         // x = spike strength (0 = off), y = spike length in sigmas,
                       // w = hidden point index + 1 (0 = none)
} ubo;

// Flat vec4 layout (2 vec4 per point) - MoltenVK-safe.
layout(std430, set = 1, binding = 0) readonly buffer PointBuffer
{
    vec4 data[];
} u_Points;

const int POINT_VEC4S = 2;

// The Gaussian's sigma as a fraction of the star's nominal size, and how many
// sigma of quad we hand the fragment stage. The quad has to hold the WIDE skirt
// (see the frag) or the profile gets cut off with a step in it; 4.5 sigma leaves
// ~2% at the edge, which the frag windows away.
const float SIGMA_FRAC  = 0.32;
const float QUAD_SIGMAS = 4.5;

// Diffraction spikes on the brightest stars only. peak is the compressed
// magnitude law (1.0 at mag ~4.4); the ramp starts around mag 2.5 and is full
// by about mag 0.7, so a handful of stars per sky get them.
const float SPIKE_MIN   = 2.5;
const float SPIKE_RANGE = 3.5;

// Brightness ceiling. The compressed magnitude law is fine across the
// catalogue but Sol is in it at magnitude -26.7 - a peak of ~4 million,
// which painted its whole quad white and bloomed into a second sun.
const float PEAK_MAX = 40.0;

layout(location = 0) in vec2 inCorner; // [-0.5, 0.5]
layout(location = 1) in vec2 inUV;     // [0, 1]

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColour;
layout(location = 2) out float vHalfPx; // quad half-extent in pixels
layout(location = 3) out float vSigmaPx;
layout(location = 4) out vec2 vSpike;   // x = strength 0..1, y = half-length px

void main()
{
    int base     = gl_InstanceIndex * POINT_VEC4S;
    vec4 posSize = u_Points.data[base + 0]; // xyz = world pos, w = size in px
    vec4 colour  = u_Points.data[base + 1]; // rgb = hue (max component 1), a = peak

    vec3 center   = posSize.xyz;
    float distCam = length(center - ubo.camPos.xyz);

    vec4 clip = ubo.projView * vec4(center, 1.0);

    // A star that lands on a sigma much under a pixel is what twinkles: as it
    // slides across pixel centres its PEAK pixel swings 24% at sigma 1, 37% at
    // 0.8 (measured over sub-pixel offsets). So sigma has a floor - and rather
    // than let the floor make faint stars brighter, the peak pays for the extra
    // width so TOTAL ENERGY is unchanged. A star too small to be stable comes
    // out dimmer and slightly softer instead of flashing, which is also what it
    // should look like.
    float sigma0 = posSize.w * SIGMA_FRAC;
    float sigma  = max(sigma0, ubo.screenParams.w);
    float energy = (sigma0 * sigma0) / (sigma * sigma); // 1 when not floored
    energy       = min(energy, 1.0);

    float halfPx = sigma * QUAD_SIGMAS;

    // Spikes need room past the profile's window, so the quad grows for them.
    // The core is windowed at QUAD_SIGMAS in pixels regardless of quad size.
    float peak     = min(colour.a, PEAK_MAX);
    float spikeAmt = clamp((peak - SPIKE_MIN) / SPIKE_RANGE, 0.0, 1.0) * ubo.look.x;
    float spikeLen = 0.0;
    if(spikeAmt > 0.001)
    {
        spikeLen = sigma * ubo.look.y * (0.5 + spikeAmt);
        halfPx   = max(halfPx, spikeLen);
    }

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

    // Apparent magnitude already accounts for how far a star is FROM EARTH, so
    // this is only about flying away from Sol: neutral for anything past ~6pc,
    // brightening as you actually close on one.
    float atten = clamp(36.0 / (distCam * distCam + 1e-4), 1.0, 3.0);
    float skyFade = clamp(ubo.camPos.w, 0.0, 1.0);
    // The star a 3D sphere is currently standing in for draws nothing.
    int hidden = int(ubo.look.w + 0.5);
    if(hidden > 0 && gl_InstanceIndex == hidden - 1)
        skyFade = 0.0;

    vUV      = inUV;
    vHalfPx  = halfPx;
    vSigmaPx = sigma;
    vSpike   = vec2(spikeAmt, spikeLen);
    // In-system (sky fade < 1) the ceiling drops with the square of the fade:
    // the star you are AT is a sphere now, and its sprite - or any bright
    // neighbour's - must not read as a second sun off to one side.
    float ceiling = PEAK_MAX * skyFade * skyFade;
    vColour  = vec4(colour.rgb * min(peak * atten, ceiling) * energy * skyFade * dim, 1.0);
}
