#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UBO
{
    mat4 projView;
    vec4 camPos;       // xyz = camera world pos, w = time (seconds)
    vec4 camRight;
    vec4 camUp;
    vec4 screenParams; // x = width px, y = height px, z = tan(fov/2), w = min planet px
} ubo;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) flat in vec4 vPosRadius; // xyz centre, w radius
layout(location = 2) flat in vec4 vStar;      // xyz star pos, w type
layout(location = 3) flat in vec4 vTint;      // rgb base colour, w seed
layout(location = 4) flat in vec4 vParams;    // x tilt, y cloud, z spin, w rings
layout(location = 5) flat in vec2 vQuad;      // x = quad half size (world), y = upscale (1 = true size)

layout(location = 0) out vec4 outColour;

// ---- hash / noise ----
float hash(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float vnoise(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)), f.x),
                   mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
                   mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}

float fbm(vec3 p)
{
    float a = 0.5, s = 0.0;
    for(int i = 0; i < 5; i++)
    {
        s += a * vnoise(p);
        p *= 2.02;
        a *= 0.5;
    }
    return s;
}

vec4 glowOut(vec3 col, float amt)
{
    float a = clamp(amt, 0.0, 1.0);
    return vec4(col * (amt / max(a, 1e-4)), a);
}

vec3 rotY(vec3 p, float a)
{
    float c = cos(a), s = sin(a);
    return vec3(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
}

vec3 rotZ(vec3 p, float a)
{
    float c = cos(a), s = sin(a);
    return vec3(c * p.x - s * p.y, s * p.x + c * p.y, p.z);
}

// ---- surfaces ----
vec3 terrestrial(vec3 n, vec3 tint, float seed, float t, float spin, float cloud)
{
    vec3 sp = rotY(n, t * spin) + seed;
    float land = fbm(sp * 2.3);
    land += 0.5 * fbm(sp * 5.1);

    float lat = abs(n.y);
    // Ocean vs land.
    float sea = smoothstep(0.62, 0.66, land);
    vec3 ocean = vec3(0.03, 0.16, 0.34) * (0.8 + 0.4 * tint);
    vec3 lowLand = vec3(0.13, 0.32, 0.12);
    vec3 highLand = vec3(0.42, 0.35, 0.22);
    vec3 ground = mix(lowLand, highLand, smoothstep(0.66, 0.82, land));
    ground = mix(ground, ground * (0.7 + 0.6 * tint), 0.5);
    vec3 surf = mix(ocean, ground, sea);

    // Polar ice.
    float ice = smoothstep(0.78, 0.9, lat + 0.15 * land);
    surf = mix(surf, vec3(0.92, 0.95, 1.0), ice);

    vec3 cp = rotY(n, t * spin * 1.3) + seed * 1.7;
    float clouds = smoothstep(0.58, 0.8, fbm(cp * 2.8)) * cloud;
    vec3 cloudCol = mix(vec3(0.95), tint, 0.35);
    surf = mix(surf, cloudCol, clouds * 0.75);
    return surf;
}

// Airless rocky body (moons, asteroids): cratered grey regolith, no ocean/clouds.
vec3 rocky(vec3 n, vec3 tint, float seed, float t, float spin)
{
    vec3 sp = rotY(n, t * spin) + seed;
    float h = fbm(sp * 3.1) + 0.5 * fbm(sp * 7.7);

    // Crater-ish dark pocks: cellular-feel from thresholded high-freq noise.
    float pock = smoothstep(0.62, 0.78, vnoise(sp * 11.0));
    vec3 surf = tint * (0.55 + 0.5 * h);
    surf = mix(surf, surf * 0.55, pock);

    // Big mare-like patches.
    float mare = smoothstep(0.6, 0.72, fbm(sp * 1.7 + 3.7));
    surf = mix(surf, surf * 0.65, mare * 0.8);
    return surf;
}

vec3 gasGiant(vec3 n, vec3 tint, float seed, float t, float spin, float contrast)
{
    float k = contrast <= 0.001 ? 0.6 : contrast;
    vec3 wp = rotY(n, t * spin) + seed;
    float warp = fbm(wp * 3.0) * 0.35;
    float bands = sin((n.y + warp) * mix(9.0, 17.0, k));
    float b = 0.5 + 0.5 * bands;
    float warm = smoothstep(0.0, 0.15, tint.r - tint.b);
    vec3 zone = mix(tint, vec3(0.95, 0.90, 0.80), 0.25 * warm);
    vec3 belt = tint * mix(vec3(0.55), vec3(0.62, 0.45, 0.33), warm);
    vec3 surf = mix(zone, belt, smoothstep(0.5 - 0.35 * k, 0.5 + 0.35 * k, b) * k);
    surf *= 1.0 - 0.25 * k + 0.5 * k * fbm(wp * 8.0);

    vec3 spot = normalize(vec3(0.6, -0.25, 0.75));
    float d = distance(rotY(n, t * spin), spot);
    vec3 stormCol = mix(tint * 0.55, vec3(0.78, 0.42, 0.30), warm);
    surf = mix(surf, stormCol, smoothstep(0.28, 0.10, d) * (0.25 + 0.6 * warm) * k);
    return surf;
}

void main()
{
    vec3 center  = vPosRadius.xyz;
    float radius = vPosRadius.w;

    if(vQuad.y > 1.001)
    {
        float d = length(vWorldPos - center) / vQuad.x; // 0 at centre, 1 at quad edge
        int dotType = int(vStar.w + 0.5);
        vec4 clipD = ubo.projView * vec4(center, 1.0);
        gl_FragDepth = clipD.z / clipD.w;

        if(dotType == 3)
        {
            if(d > 1.0)
                discard;
            vec3 offD  = vWorldPos - center;
            float qx   = dot(offD, ubo.camRight.xyz) / vQuad.x;
            float qy   = dot(offD, ubo.camUp.xyz) / vQuad.x;
            float core = smoothstep(0.34, 0.0, d);
            float halo = exp(-d * 4.5) * 0.5;
            float arms = (exp(-abs(qy) * 14.0) * exp(-abs(qx) * 1.8)
                        + exp(-abs(qx) * 14.0) * exp(-abs(qy) * 1.8)) * 0.30;
            float amt  = core * 2.0 + halo + arms;
            if(amt < 0.01)
                discard;
            outColour = glowOut(vTint.rgb, amt);
            return;
        }

        if(d > 0.85)
            discard;
        float core = smoothstep(0.85, 0.35, d);
        outColour = glowOut(vTint.rgb * 0.85, core);
        return;
    }

    vec3 ro = ubo.camPos.xyz;
    vec3 rd = normalize(vWorldPos - ro);

    // Ray vs sphere.
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;

    float t    = 0.0; // planets don't spin - real-time rotation grows unbounded and gets juttery (float precision) at high sim speed
    int type   = int(vStar.w + 0.5);
    float seed = vTint.w;
    float spin = vParams.z;
    vec3 V     = -rd;

    if(type == 3)
    {
        float tHitS = (disc >= 0.0) ? (-b - sqrt(disc)) : -1.0;
        if(tHitS >= 0.0)
        {
            vec3 hitP = ro + rd * tHitS;
            vec3 N    = normalize(hitP - center);
            vec3 sp   = rotY(N, t * spin * 0.3) + seed;
            float mu  = clamp(dot(N, V), 0.0, 1.0); // 1 at disc centre, 0 at the limb

            float superg = fbm(sp * 3.5);
            float gran   = fbm(sp * 16.0 + superg);
            float lanes  = smoothstep(0.30, 0.62, gran);
            float cells  = mix(0.74, 1.13, lanes) * (0.94 + 0.12 * superg);

            float belt  = exp(-pow((abs(N.y) - 0.32) / 0.21, 2.0));
            float spotN = fbm(sp * 5.0 + 11.0);
            float spot  = smoothstep(0.60, 0.71, spotN) * belt; // penumbra
            float umbra = smoothstep(0.67, 0.75, spotN) * belt; // darker core

            float fac = smoothstep(0.56, 0.76, fbm(sp * 9.0 + 4.0)) * (1.0 - mu) * belt;

            vec3 scol = vTint.rgb * cells * (0.42 + 0.58 * mu);
            scol = mix(scol, scol * vec3(1.15, 0.80, 0.55), (1.0 - mu) * 0.55);
            scol *= 1.0 + 0.6 * fac;
            scol = mix(scol, scol * 0.22, spot * 0.9);
            scol = mix(scol, scol * 0.30, umbra * 0.95);

            vec4 clipS = ubo.projView * vec4(hitP, 1.0);
            gl_FragDepth = clipS.z / clipS.w;
            outColour = vec4(scol * 1.25, 1.0);
            return;
        }

        float tC = -b;
        if(tC <= 0.0)
            discard;
        vec3 closeP = ro + rd * tC;
        vec3 off    = closeP - center;
        float dc    = length(off) / radius; // 1 at the limb, ~4.5 at quad edge
        if(dc <= 1.0)
            discard;
        vec3 viewD  = normalize(center - ro);
        vec3 a1     = normalize(cross(vec3(0.0, 1.0, 0.0), viewD) + vec3(1e-5));
        vec3 a2     = normalize(cross(viewD, a1));
        float ax    = dot(off, a1) / radius;
        float ay    = dot(off, a2) / radius;

        // Chromosphere: a thin warm rim hugging the limb, just above the surface.
        float chromo = exp(-(dc - 1.0) * 24.0) * 0.85;

        float streamers = 0.55 + 0.85 * smoothstep(0.35, 0.72, fbm(normalize(off) * 5.0 + seed * 3.0));
        float corona = exp(-(dc - 1.0) * 2.2) * 0.5 * streamers;
        corona += exp(-(dc - 1.0) * 0.55) * 0.07;

        float arms = exp(-abs(ay) * 6.0) * exp(-abs(ax) * 1.15)
                   + exp(-abs(ax) * 6.0) * exp(-abs(ay) * 1.15);
        arms *= 0.32;

        float edgeFade = 1.0 - smoothstep(2.9, 4.35, dc);
        corona *= edgeFade;
        arms   *= edgeFade;

        float glow = corona + arms + chromo;
        if(glow < 0.004)
            discard;
        // Warm chromosphere mixed into the (cooler) coronal tint by its share.
        vec3 fcol = mix(vTint.rgb * 1.6, vec3(1.0, 0.42, 0.22) * 1.3,
                        clamp(chromo / max(glow, 1e-4), 0.0, 1.0));
        vec4 clipF = ubo.projView * vec4(closeP, 1.0);
        gl_FragDepth = clipF.z / clipF.w;
        outColour = glowOut(fcol, glow);
        return;
    }

    // Resolve procedural type from the seed.
    if(type == 2)
        type = (hash(vec3(seed)) > 0.45) ? 1 : 0;

    // ---- sphere hit ----
    bool sphereHit = false;
    float tHit = 0.0;
    vec3 hitP, N;
    if(disc >= 0.0)
    {
        float tCandidate = -b - sqrt(disc);
        if(tCandidate >= 0.0)
        {
            sphereHit = true;
            tHit       = tCandidate;
            hitP       = ro + rd * tHit;
            N          = normalize(hitP - center);
        }
    }

    bool hasRing = vParams.w > 0.5;
    bool ringHit = false;
    float tRing  = 0.0;
    vec3 ringP, ringN = vec3(0.0, 1.0, 0.0);
    float ringInner = radius * 1.6;
    float ringOuter = radius * 2.3;
    if(hasRing)
    {
        float tilt = vParams.x;
        ringN = normalize(vec3(sin(tilt), cos(tilt), 0.0));
        float denom = dot(rd, ringN);
        if(abs(denom) > 1e-5)
        {
            float tp = dot(center - ro, ringN) / denom;
            if(tp > 0.0)
            {
                vec3 P = ro + rd * tp;
                float d = length(P - center);
                if(d >= ringInner && d <= ringOuter)
                {
                    ringHit = true;
                    tRing    = tp;
                    ringP    = P;
                }
            }
        }
    }

    // Sphere wins whenever it's nearer than (or the only) hit.
    if(sphereHit && (!ringHit || tHit <= tRing))
    {
        vec3 Ns = rotZ(N, -vParams.x);
        vec3 base;
        if(type == 1)
            base = gasGiant(Ns, vTint.rgb, seed, t, spin, vParams.y);
        else if(type == 4)
            base = rocky(Ns, vTint.rgb, seed, t, spin);
        else
            base = terrestrial(Ns, vTint.rgb, seed, t, spin, vParams.y);

        // Lighting from the system star.
        vec3 L = normalize(vStar.xyz - hitP);
        float ndl = max(dot(N, L), 0.0);
        float wrap = (ndl + 0.15) / 1.15;        // soft terminator

        vec3 col = base * (0.16 + 0.84 * clamp(wrap, 0.0, 1.0));

        // Ocean/ice specular for terrestrial.
        if(type == 0)
        {
            vec3 H = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0), 60.0) * ndl;
            col += spec * 0.25;
        }

        float ndv  = max(dot(N, V), 0.0);
        float fres = pow(1.0 - ndv, 3.0);
        vec3 atmoTint = (type == 1) ? vTint.rgb * 0.6 + 0.2 : vec3(0.35, 0.55, 1.0);
        float atmoAmt = (type == 4) ? 0.12 : 0.9;
        if(type == 4)
            atmoTint = vTint.rgb;
        col += atmoTint * fres * clamp(wrap + 0.2, 0.0, 1.0) * atmoAmt;
        if(type != 4)
        {
            float fres2 = pow(1.0 - ndv, 8.0);
            col += atmoTint * fres2 * clamp(wrap + 0.35, 0.0, 1.0) * atmoAmt * 0.8;
            if(type == 0)
            {
                float term = smoothstep(0.30, 0.02, abs(wrap - 0.16));
                col += vec3(1.0, 0.45, 0.22) * fres * term * 0.35;
            }
        }

        // Keep out of bloom-blowout territory (planets render into the HDR target).
        col *= 0.8;

        // Correct depth from the sphere hit so planets occlude orbits / each other.
        vec4 clip = ubo.projView * vec4(hitP, 1.0);
        gl_FragDepth = clip.z / clip.w;

        outColour = vec4(col, 1.0);
        return;
    }

    if(ringHit)
    {
        vec3 L = normalize(vStar.xyz - ringP);
        float ndl = abs(dot(ringN, L));
        float dNorm = (length(ringP - center) - ringInner) / max(ringOuter - ringInner, 1e-4);
        float band = 0.55 + 0.45 * fbm(vec3(dNorm * 9.0, seed * 3.1, 0.0));
        // Fade both edges so the band doesn't hard-cut.
        float edgeFade = smoothstep(0.0, 0.08, dNorm) * (1.0 - smoothstep(0.88, 1.0, dNorm));
        vec3 ringCol = vTint.rgb * band * (0.65 + 0.35 * ndl);

        vec4 clipR = ubo.projView * vec4(ringP, 1.0);
        gl_FragDepth = clipR.z / clipR.w;
        outColour = glowOut(ringCol, edgeFade);
        return;
    }

    {
        float tClosest = -b; // ray parameter of closest approach to the centre
        if(tClosest > 0.0)
        {
            vec3 closeP = ro + rd * tClosest;
            float distClosest = length(closeP - center);
            float haloR = radius * 1.45;
            if(distClosest > radius && distClosest < haloR)
            {
                float haloFade = 1.0 - smoothstep(radius, haloR, distClosest);
                haloFade = haloFade * haloFade;
                vec3 atmoTint = (type == 1) ? vTint.rgb * 0.6 + 0.2
                              : (type == 4) ? vTint.rgb * 0.7 + 0.12
                              :               vec3(0.4, 0.6, 1.0);
                float amt = (type == 4) ? 0.5 : 1.05;
                float apparent = radius / max(length(oc), radius); // ~angular size
                amt *= mix(1.0, 0.18, smoothstep(0.02, 0.25, apparent));
                vec4 clipH = ubo.projView * vec4(closeP, 1.0);
                gl_FragDepth = clipH.z / clipH.w;
                outColour = glowOut(atmoTint, haloFade * amt);
                return;
            }
        }
    }

    discard;
}
