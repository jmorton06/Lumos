#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 Reproj;   // this frame's unjittered NDC -> last frame's clip
    vec4 Jitter;   // xy = this frame's jitter in NDC, zw = 1 / size
    vec4 Params;   // x = history weight still, y = history weight moving, z = 1 resets history, w = variance clip gamma
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Current;
layout(set = 0, binding = 2) uniform sampler2D u_History;
layout(set = 0, binding = 3) uniform sampler2D u_Depth;

layout(location = 0) out vec4 outColour;
layout(location = 1) out vec4 outHistory;

vec3 RGBToYCoCg(vec3 c)
{
    return vec3(dot(c, vec3(0.25, 0.5, 0.25)), dot(c, vec3(0.5, 0.0, -0.5)), dot(c, vec3(-0.25, 0.5, -0.25)));
}

vec3 YCoCgToRGB(vec3 c)
{
    return vec3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

// Karis: blend in a tone mapped space so one bright pixel can't flicker the whole neighbourhood
float LumaWeight(vec3 c)
{
    return 1.0 / (1.0 + max(c.r, max(c.g, c.b)));
}

// 9 taps folded into 5 bilinear ones; sharper than plain bilinear, which blurs the history every frame
vec3 SampleCatmullRom(sampler2D tex, vec2 uv, vec2 texSize)
{
    vec2 samplePos = uv * texSize;
    vec2 texPos1   = floor(samplePos - 0.5) + 0.5;
    vec2 f         = samplePos - texPos1;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12      = w1 + w2;
    vec2 offset12 = w2 / w12;

    vec2 texPos0  = (texPos1 - 1.0) / texSize;
    vec2 texPos3  = (texPos1 + 2.0) / texSize;
    vec2 texPos12 = (texPos1 + offset12) / texSize;

    vec3 result = vec3(0.0);
    result += textureLod(tex, vec2(texPos12.x, texPos0.y), 0.0).rgb * w12.x * w0.y;
    result += textureLod(tex, vec2(texPos0.x, texPos12.y), 0.0).rgb * w0.x * w12.y;
    result += textureLod(tex, vec2(texPos12.x, texPos12.y), 0.0).rgb * w12.x * w12.y;
    result += textureLod(tex, vec2(texPos3.x, texPos12.y), 0.0).rgb * w3.x * w12.y;
    result += textureLod(tex, vec2(texPos12.x, texPos3.y), 0.0).rgb * w12.x * w3.y;

    float norm = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    return max(result / norm, vec3(0.0));
}

// Pulls the history towards the box centre until it is inside (Playdead's clip, not a clamp: no colour shift on the box edge)
vec3 ClipAABB(vec3 boxMin, vec3 boxMax, vec3 history)
{
    vec3 centre = 0.5 * (boxMax + boxMin);
    vec3 extent = 0.5 * (boxMax - boxMin) + 1e-5;
    vec3 v      = history - centre;
    vec3 units  = abs(v / extent);
    float m     = max(units.x, max(units.y, units.z));
    return m > 1.0 ? centre + v / m : history;
}

void main()
{
    ivec2 size  = textureSize(u_Current, 0);
    ivec2 pixel = clamp(ivec2(outTexCoord * vec2(size)), ivec2(0), size - 1);

    vec3 current = texelFetch(u_Current, pixel, 0).rgb;

    // neighbourhood stats + the nearest depth around the pixel (silhouettes take the foreground's motion)
    vec3 m1          = vec3(0.0);
    vec3 m2          = vec3(0.0);
    float nearest    = 0.0; // reversed-Z: larger = nearer
    ivec2 nearestPix = pixel;
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            ivec2 p = clamp(pixel + ivec2(x, y), ivec2(0), size - 1);
            vec3 c  = RGBToYCoCg(texelFetch(u_Current, p, 0).rgb);
            m1 += c;
            m2 += c * c;

            float d = texelFetch(u_Depth, p, 0).r;
            if(d > nearest)
            {
                nearest    = d;
                nearestPix = p;
            }
        }
    }

    if(ubo.Params.z > 0.5 || any(isnan(current)))
    {
        current    = any(isnan(current)) ? vec3(0.0) : current;
        outColour  = vec4(current, 1.0);
        outHistory = outColour;
        return;
    }

    // the depth was drawn jittered: take the jitter back off before reprojecting with the plain matrices
    vec2 nearestUV = (vec2(nearestPix) + 0.5) * ubo.Jitter.zw;
    vec4 ndc       = vec4(nearestUV * 2.0 - 1.0 - ubo.Jitter.xy, max(nearest, 1e-7), 1.0);
    vec4 prevClip  = ubo.Reproj * ndc;
    vec2 velocity  = vec2(0.0);
    if(prevClip.w > 1e-6)
        velocity = (prevClip.xy / prevClip.w * 0.5 + 0.5) - (nearestUV - ubo.Jitter.xy * 0.5);

    vec2 historyUV = outTexCoord + velocity;
    if(any(lessThan(historyUV, vec2(0.0))) || any(greaterThan(historyUV, vec2(1.0))))
    {
        outColour  = vec4(current, 1.0);
        outHistory = outColour;
        return;
    }

    vec3 history = SampleCatmullRom(u_History, historyUV, vec2(size));

    vec3 mean   = m1 / 9.0;
    vec3 sigma  = sqrt(abs(m2 / 9.0 - mean * mean));
    float gamma = ubo.Params.w;
    vec3 boxMin = mean - gamma * sigma;
    vec3 boxMax = mean + gamma * sigma;

    history = YCoCgToRGB(ClipAABB(boxMin, boxMax, RGBToYCoCg(history)));

    // more of this frame the faster the pixel moves: history smears under motion
    float speed  = length(velocity * vec2(size));
    float weight = mix(ubo.Params.x, ubo.Params.y, clamp(speed / 8.0, 0.0, 1.0));

    float wc       = (1.0 - weight) * LumaWeight(current);
    float wh       = weight * LumaWeight(history);
    vec3 resolved  = (current * wc + history * wh) / max(wc + wh, 1e-5);

    outColour  = vec4(resolved, 1.0);
    outHistory = outColour;
}
