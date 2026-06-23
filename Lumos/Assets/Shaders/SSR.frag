#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Octahedral.glslh"

// Screen-space reflections. Reads a packed G-buffer (rg = octahedral world
// normal, b = NDC depth, a = roughness) so it needs no separate depth/normal
// targets and works whether or not MSAA resolved them. For each pixel:
// reconstruct view-space position + normal, reflect the view ray, linearly
// march the depth buffer, binary-refine the crossing, then blend in the lit
// colour at the hit. Roughness widens the reflection (mip-style blur) and
// fades it out; very rough pixels are skipped entirely. Where the screen ray
// misses or runs off-frame it falls back to the prefiltered environment map
// (the same IBL the forward pass uses), faded by hit confidence.

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 projection;
    mat4 invProj;
    mat4 view;
    float Near;
    float Far;
    float MaxDistance;
    float Thickness;
    int MaxSteps;
    int BinarySteps;
    float Strength;
    float MaxRoughness;
    float EnvMipCount;     // 0 = no environment, env fallback disabled
    float EnvIntensity;
    float _pad0;
    float _pad1;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Colour;
layout(set = 0, binding = 2) uniform sampler2D u_GBuffer;
layout(set = 0, binding = 3) uniform samplerCube u_EnvMap;

layout(location = 0) out vec4 outFrag;

// Vulkan clip space: depth is NDC z directly in [0,1], xy needs *2-1.
vec3 ReconstructVS(vec2 uv, float depth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 vp   = ubo.invProj * clip;
    return vp.xyz / vp.w;
}

// View-space point -> screen UV via projection.
vec2 ProjectUV(vec3 vsPos)
{
    vec4 clip = ubo.projection * vec4(vsPos, 1.0);
    return (clip.xy / clip.w) * 0.5 + 0.5;
}

float SceneDepth(vec2 uv) { return texture(u_GBuffer, uv).b; }

// On-screen reflections are kept sharp (clean mirror look). The roughness blur
// comes from the environment-map mip in the fallback, not by smearing the
// screen sample — smearing was what made glossy floors look dirty.
vec3 SampleReflection(vec2 uv)
{
    return texture(u_Colour, uv).rgb;
}

// Per-pixel dither so the coarse march bands break up into noise instead of
// visible stair-steps.
float Dither(vec2 p)
{
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))));
}

void main()
{
    vec4 sceneColour = texture(u_Colour, outTexCoord);
    outFrag = sceneColour;

    vec4 gb         = texture(u_GBuffer, outTexCoord);
    float depth     = gb.b;
    float roughness = gb.a;

    // No geometry here (cleared G-buffer reads ~0), or too rough to reflect.
    if(depth <= 0.0 || depth >= 1.0)
        return;
    if(roughness > ubo.MaxRoughness)
        return;

    vec3 posVS    = ReconstructVS(outTexCoord, depth);
    vec3 normalVS = normalize(mat3(ubo.view) * OctDecodeNormal(gb.rg));
    vec3 viewDir  = normalize(posVS);            // camera at origin in VS
    vec3 reflVS   = reflect(viewDir, normalVS);

    bool hasEnv = ubo.EnvMipCount > 0.5;

    // Screen-space march only makes sense for rays going into the scene; rays
    // pointing back toward the camera (z > 0) have no screen data, but the
    // environment map still covers that direction.

    // Small view-space bias so the ray doesn't immediately hit its own surface.
    const float kBias = 0.025;
    // Dither the march start by up to one step so the coarse sampling banding
    // turns into fine noise instead of visible stair-steps.
    float jitter = Dither(gl_FragCoord.xy);

    vec2 hitUV    = vec2(0.0);
    vec3 hitPos   = posVS;
    bool hit      = false;
    bool wasFront = true;     // previous sample was in front of the surface
    vec3 prevPos  = posVS;
    float prevDist = 0.0;

    if(reflVS.z < 0.0)
    {
        for(int i = 1; i <= ubo.MaxSteps; ++i)
        {
            // Quadratic spacing: fine steps near the surface (where reflected
            // detail lives and overshoot causes gaps/jaggies), coarse far away.
            float t    = clamp((float(i) - jitter) / float(ubo.MaxSteps), 0.0, 1.0);
            float dist = t * t * ubo.MaxDistance;
            float segLen = dist - prevDist;

            vec3 rayPos = posVS + reflVS * dist;

            vec2 uv = ProjectUV(rayPos);
            if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
                break;

            float sd = SceneDepth(uv);
            if(sd <= 0.0)
            {
                prevPos  = rayPos;
                prevDist = dist;
                wasFront = true;   // over sky: no surface to be behind of
                continue;
            }

            vec3 sceneVS = ReconstructVS(uv, sd);
            float diff   = sceneVS.z - rayPos.z;   // >0: ray is behind the surface

            // Accept a crossing as a real surface only within this depth window,
            // tied to the local segment length so a single step's overshoot is
            // always inside it — that's what stops gaps in thin geometry.
            float acceptThickness = max(ubo.Thickness, segLen * 2.0);

            // Front -> behind transition means the ray crossed a surface between
            // prevPos and rayPos. Refine to the crossing, then validate the gap.
            if(diff > kBias && wasFront)
            {
                vec3 a = prevPos;
                vec3 b = rayPos;
                for(int j = 0; j < ubo.BinarySteps; ++j)
                {
                    vec3 mid = (a + b) * 0.5;
                    vec3 mvs = ReconstructVS(ProjectUV(mid), SceneDepth(ProjectUV(mid)));
                    if(mvs.z - mid.z > 0.0)
                        b = mid;   // still behind
                    else
                        a = mid;   // in front
                }

                if(diff < acceptThickness)
                {
                    hitUV  = ProjectUV(b);
                    hitPos = b;
                    hit    = true;
                    break;
                }
                // Otherwise we punched through a thin gap to the background; keep going.
                wasFront = false;
            }
            else
            {
                wasFront = diff <= kBias;
            }

            prevPos  = rayPos;
            prevDist = dist;
        }
    }

    // Nothing to add: no screen hit and no environment to fall back to.
    if(!hit && !hasEnv)
        return;

    // Gentle grazing boost only — never crush head-on reflections (that was the
    // "fades out too fast" bug). Head-on keeps ~0.75, grazing rises to 1.0.
    float NoV          = clamp(dot(-viewDir, normalVS), 0.0, 1.0);
    float grazingBoost = mix(0.75, 1.0, pow(1.0 - NoV, 4.0));

    // Keep reflections strong across most of the roughness range, only rolling
    // off in the last stretch before MaxRoughness. The old curve started at 0
    // so a 0.5-roughness floor was already ~95% gone — that read as "no/dirty
    // reflection". Now it stays near full until ~0.6*MaxRoughness.
    float roughFade = 1.0 - smoothstep(ubo.MaxRoughness * 0.6, ubo.MaxRoughness, roughness);

    // Confidence in the screen-space hit: full in the centre, fading at screen
    // edges and at the end of the march. Where it drops, we lean on the env map.
    float screenConf = 0.0;
    if(hit)
    {
        vec2 e         = smoothstep(0.0, 0.12, hitUV) * (1.0 - smoothstep(0.88, 1.0, hitUV));
        float edgeFade = e.x * e.y;
        float hitDist  = length(hitPos - posVS);
        float distFade = 1.0 - smoothstep(ubo.MaxDistance * 0.75, ubo.MaxDistance, hitDist);
        screenConf     = edgeFade * distFade;
    }

    vec3 screenRefl = SampleReflection(hitUV);

    vec3 reflColour;
    float amount;
    if(hasEnv)
    {
        // Prefiltered environment, roughness-selected mip — matches the forward
        // IBL lookup so the fallback is continuous with on-screen reflections.
        mat3 invViewRot = transpose(mat3(ubo.view));
        vec3 reflW      = normalize(invViewRot * reflVS);
        float maxMip    = max(ubo.EnvMipCount - 1.0, 1.0);
        vec3 envRefl    = textureLod(u_EnvMap, reflW, roughness * maxMip).rgb * ubo.EnvIntensity;

        reflColour = mix(envRefl, screenRefl, screenConf);
        amount     = clamp(ubo.Strength * roughFade * grazingBoost, 0.0, 1.0);
    }
    else
    {
        // No environment: screen reflection fades to the underlying surface.
        reflColour = screenRefl;
        amount     = clamp(ubo.Strength * roughFade * grazingBoost * screenConf, 0.0, 1.0);
    }

    outFrag = vec4(mix(sceneColour.rgb, reflColour, amount), sceneColour.a);
}
