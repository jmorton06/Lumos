#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UBO
{
    mat4 projView;
    vec4 camPos;       // xyz eye for this cloud's frame, w = premultiply flag
    vec4 screenParams;
} ubo;

layout(location = 0) in vec4 vColour;
layout(location = 1) in float vSide;
layout(location = 2) in float vAlong;
layout(location = 3) in vec2 vDash;      // x = period, y = duty

layout(location = 0) out vec4 outColour;

// Cross-section profile. This is the analytic form of the 1D "profile texture"
// ribbon renderers sample across the width: a Gaussian core sitting on a low
// shoulder. Evaluating it costs less than a fetch, needs no asset or sampler
// binding, and is exact at every width - a sampled profile would need a mip
// chain to stay clean once the ribbon is a couple of pixels across.
const float PROFILE_K    = 2.6;  // core tightness; higher = thinner hot centre
const float PROFILE_EDGE = 0.30; // profile value at the stroke edge
const float CORE_BOOST   = 0.55; // extra HDR in the centre, so bloom finds it
const float FILTER_PX    = 1.3;  // edge filter width in pixels; wider = smoother, softer

void main()
{
    // Dash pattern along the path (period in world units, duty = fraction drawn).
    if(vDash.x > 0.0)
    {
        float phase = fract(vAlong / vDash.x);
        float aaP   = fwidth(vAlong / vDash.x) * 1.5;
        float onOff = 1.0 - smoothstep(vDash.y - aaP, vDash.y + aaP, phase);
        if(onOff <= 0.001)
            discard;
    }

    // vSide runs -1..1 across the ribbon, so its screen-space derivative gives
    // the quad's half-width IN PIXELS - measured from the projection that
    // actually drew it. That is what makes this frame-agnostic: the same maths
    // is right for world, star-local and camera-relative draws, none of which
    // agree on where the eye is.
    // Euclidean, not fwidth: fwidth is |dx|+|dy|, which overstates the
    // gradient by up to 1.4x on a diagonal and drew those lines thinner and
    // rougher than horizontal ones.
    vec2 grad    = vec2(dFdx(vSide), dFdy(vSide));
    float halfPx = 1.0 / max(length(grad), 1e-8);

    // vSide runs past +-1 into the quad's margin (see the vert); the profile
    // below only wants the in-stroke part.
    float t = min(abs(vSide), 1.0);

    // Coverage of a FILTER_PX-wide box filter against the ribbon edge. Using
    // the derivative for the SCALE (which varies slowly across the quad) rather
    // than for the edge distance itself is what keeps thin lines from crawling.
    float cov = clamp((1.0 - abs(vSide)) * halfPx / FILTER_PX + 0.5, 0.0, 1.0);

    // Sub-pixel ribbons can't be drawn narrower than a pixel, so let them fade
    // instead: a hairline dims away rather than breaking into dashes as the
    // camera moves.
    cov *= clamp(halfPx / 0.7, 0.0, 1.0);
    if(cov <= 0.0)
        discard;

    float gauss = exp(-PROFILE_K * t * t);
    float prof  = mix(PROFILE_EDGE, 1.0, gauss);

    float alpha = vColour.a * cov * prof;

    vec3 rgb = vColour.rgb * (1.0 + CORE_BOOST * gauss);

    // Match whatever blend the pass set up. camPos.w = 1 means premultiplied
    // over-blend (One / OneMinusSrcAlpha); 0 means the old additive pass, where
    // the hardware multiplies by alpha itself and premultiplying here would
    // square it - which is exactly the bug this shader used to have, and why
    // faint lines washed out. Keeping both paths lets one .spv be correct
    // before and after the engine rebuild that flips the pass over.
    if(ubo.camPos.w > 0.5)
        rgb *= alpha;

    outColour = vec4(rgb, alpha);
}
