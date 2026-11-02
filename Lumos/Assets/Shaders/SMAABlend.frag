#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 1) uniform sampler2D u_Colour;
layout(set = 0, binding = 2) uniform sampler2D u_Weights;

void main()
{
    vec2 rt = 1.0 / vec2(textureSize(u_Colour, 0));

    float up    = texture(u_Weights, uv).r;
    float left  = texture(u_Weights, uv).g;
    float down  = texture(u_Weights, uv + vec2(0.0, rt.y)).r; // pixel below's top edge
    float right = texture(u_Weights, uv + vec2(rt.x, 0.0)).g; // pixel right's left edge

    vec4 c = texture(u_Colour, uv);

    float horiz = max(left, right);
    float vert  = max(up, down);

    if(max(horiz, vert) < 1e-4)
    {
        outColour = c;
        return;
    }

    if(horiz >= vert)
    {
        vec3 cl = texture(u_Colour, uv - vec2(rt.x, 0.0)).rgb;
        vec3 cr = texture(u_Colour, uv + vec2(rt.x, 0.0)).rgb;
        float total = left + right;
        vec3 blended = (cl * left + cr * right) / max(total, 1e-5);
        c.rgb = mix(c.rgb, blended, clamp(total, 0.0, 1.0));
    }
    else
    {
        vec3 cu = texture(u_Colour, uv - vec2(0.0, rt.y)).rgb;
        vec3 cd = texture(u_Colour, uv + vec2(0.0, rt.y)).rgb;
        float total = up + down;
        vec3 blended = (cu * up + cd * down) / max(total, 1e-5);
        c.rgb = mix(c.rgb, blended, clamp(total, 0.0, 1.0));
    }

    outColour = c;
}
