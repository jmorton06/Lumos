#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColour;
layout(location = 2) in float vHalfPx;
layout(location = 3) in float vSigmaPx;
layout(location = 4) in vec2 vSpike;    // x = strength 0..1, y = half-length px

layout(location = 0) out vec4 outColour;

// Star profile. A star is a POINT SOURCE, so what matters is not how sharp its
// edge looks in a still frame but whether its brightness stays put as it slides
// across pixel centres while the camera turns. A Gaussian is shift-invariant,
// which is why the profile is one; how much it still shimmers is set entirely by
// sigma (24% peak swing at 1px, 12% at 1.4, 6% at 2 - measured), and the vertex
// stage floors sigma for that reason.
//
// The pixel scale comes in as a VARYING rather than out of fwidth(). fwidth of a
// radius is unstable at the centre of the sprite - exactly the pixel whose value
// the eye reads as the star - and that instability was its own source of
// twinkle. The vertex stage knows the sprite's pixel size exactly.
const float WIDE_MUL    = 2.3;  // faint wide skirt at this multiple of sigma...
const float WIDE_W      = 0.14; // ...at this weight. It is what feeds bloom.
const float QUAD_SIGMAS = 4.5;  // must match the vertex stage
const float GAIN        = 1.0;
const float SPIKE_GAIN  = 0.30; // spike brightness relative to the star's peak
const float SPIKE_W     = 0.55; // spike half-width as a fraction of sigma

void main()
{
    vec2 d  = vUV * 2.0 - 1.0;
    float r = length(d);

    float rpx   = r * vHalfPx;
    float s2    = 2.0 * vSigmaPx * vSigmaPx;
    float core  = exp(-(rpx * rpx) / s2);
    float wide  = exp(-(rpx * rpx) / (s2 * WIDE_MUL * WIDE_MUL));

    // The profile has to reach zero exactly where the quad stops, or it ends on
    // a step - and a step is a hard edge that aliases and shimmers however soft
    // the rest of the sprite is. Subtract whatever the profile is worth at the
    // quad's edge (constant: the quad is always QUAD_SIGMAS across).
    const float e2   = QUAD_SIGMAS * QUAD_SIGMAS * 0.5;
    const float edge = exp(-e2) + WIDE_W * exp(-e2 / (WIDE_MUL * WIDE_MUL));

    float profile = max(core + wide * WIDE_W - edge, 0.0) / (1.0 - edge);

    // Four-point diffraction spikes along the screen axes, Gaussian across and
    // tapering to nothing at their tips, which sit inside the quad by construction.
    float spike = 0.0;
    if(vSpike.y > 0.0)
    {
        vec2 px     = d * vHalfPx;
        float sw    = max(vSigmaPx * SPIKE_W, 0.8);
        float s2w   = 2.0 * sw * sw;
        float tapX  = max(1.0 - abs(px.x) / vSpike.y, 0.0);
        float tapY  = max(1.0 - abs(px.y) / vSpike.y, 0.0);
        spike       = exp(-(px.y * px.y) / s2w) * tapX * tapX
                    + exp(-(px.x * px.x) / s2w) * tapY * tapY;
        spike      *= vSpike.x * SPIKE_GAIN;
    }

    // Brightness and hue are both baked into vColour by the vertex stage.
    outColour = vec4(vColour.rgb * (profile * GAIN + spike), 1.0);
}
