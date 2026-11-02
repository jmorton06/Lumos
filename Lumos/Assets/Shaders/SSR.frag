#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Octahedral.glslh"
#include "GBuffer.glslh"


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

vec3 SampleReflection(vec2 uv)
{
    return texture(u_Colour, uv).rgb;
}

// Karis' analytic fit of the split-sum BRDF LUT: x = F0 scale, y = bias
vec2 EnvBRDFApprox(float perceptualRoughness, float NoV)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r        = perceptualRoughness * c0 + c1;
    float a004    = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

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
    vec2 material   = GBufferUnpackMaterial(gb.a);
    float roughness = material.x;
    float metallic  = material.y;

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


    // Small view-space bias so the ray doesn't immediately hit its own surface.
    const float kBias = 0.025;
    float jitter = Dither(gl_FragCoord.xy);

    vec2 hitUV    = vec2(0.0);
    vec3 hitPos   = posVS;
    bool hit      = false;
    bool wasFront = true;     // previous sample was in front of the surface
    vec3 prevPos  = posVS;
    float prevDist = 0.0;

    // rays turned back towards the camera march too (a helmet facing us reflects the floor in
    // front of it), stopping short of the near plane
    float rayLength = ubo.MaxDistance;
    if(reflVS.z > 1e-4)
        rayLength = min(rayLength, (-ubo.Near * 1.05 - posVS.z) / reflVS.z);
    if(rayLength > 0.0)
    {
        for(int i = 1; i <= ubo.MaxSteps; ++i)
        {
            float t    = clamp((float(i) - jitter) / float(ubo.MaxSteps), 0.0, 1.0);
            float dist = t * t * rayLength;
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

            float acceptThickness = max(ubo.Thickness, segLen * 2.0);

            if(diff > kBias && wasFront)
            {
                vec3 a = prevPos;
                vec3 b = rayPos;
                for(int j = 0; j < ubo.BinarySteps; ++j)
                {
                    vec3 mid   = (a + b) * 0.5;
                    vec2 midUV = ProjectUV(mid);
                    vec3 mvs   = ReconstructVS(midUV, SceneDepth(midUV));
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

    // a miss keeps what the forward pass reflected (the environment)
    if(!hit)
        return;

    float NoV       = clamp(dot(-viewDir, normalVS), 0.0, 1.0);
    float roughFade = 1.0 - smoothstep(ubo.MaxRoughness * 0.6, ubo.MaxRoughness, roughness);

    vec2 e          = smoothstep(0.0, 0.12, hitUV) * (1.0 - smoothstep(0.88, 1.0, hitUV));
    float hitDist   = length(hitPos - posVS);
    float distFade  = 1.0 - smoothstep(ubo.MaxDistance * 0.75, ubo.MaxDistance, hitDist);
    float screenConf = e.x * e.y * distFade;

    vec3 screenRefl = SampleReflection(hitUV);

    // the forward pass already reflected the environment here: swap that for what the ray found
    vec3 envRefl = vec3(0.0);
    if(hasEnv)
    {
        mat3 invViewRot = transpose(mat3(ubo.view));
        vec3 reflW      = normalize(invViewRot * reflVS);
        float maxMip    = max(ubo.EnvMipCount - 1.0, 1.0);
        envRefl         = min(textureLod(u_EnvMap, reflW, roughness * maxMip).rgb, vec3(50.0)) * ubo.EnvIntensity;
    }

    // same split-sum weight as the forward IBL; no albedo in the G-buffer, so metals are untinted
    vec3 F0         = mix(vec3(0.04), vec3(1.0), metallic);
    vec2 ab         = EnvBRDFApprox(roughness, NoV);
    vec3 specWeight = F0 * ab.x + ab.y;

    vec3 delta = (screenRefl - envRefl) * specWeight * (screenConf * roughFade * ubo.Strength);
    outFrag    = vec4(max(sceneColour.rgb + delta, vec3(0.0)), sceneColour.a);
}
