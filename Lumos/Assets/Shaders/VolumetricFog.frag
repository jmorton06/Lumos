#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_SHADOWMAPS 4

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 InvViewProj;
    mat4 View;
    mat4 ShadowTransform[MAX_SHADOWMAPS];
    mat4 BiasMatrix;
    vec4 CameraPosition; // w = max march distance
    vec4 SunDirection;   // towards the sun, w = 1 when the shadow map is valid
    vec4 SunColour;      // exposed radiance, w = shadow cascade count
    vec4 FogColour;      // rgb = scattering colour, a = strength
    vec4 FogParams;      // x = density, y = height falloff, z = base height, w = ambient strength
    vec4 Params;         // x = anisotropy, y = steps, z = frame index, w = sun strength
    vec4 SplitDepths;
    vec4 Misc;           // x = env exposure, y = 1 when a sun is lit
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Depth;
layout(set = 0, binding = 2) uniform sampler2DArray u_ShadowMap;
layout(set = 0, binding = 3) uniform samplerCube u_IrradianceMap;

layout(location = 0) out vec4 outFog;    // rgb in-scattered light, a transmittance
layout(location = 1) out vec4 outDepth;  // r = view distance the march ended at

const float PI = 3.14159265359;

float InterleavedGradientNoise(vec2 p, float frame)
{
    p += frame * 5.588238;
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))));
}

float PhaseHG(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5));
}

float Density(vec3 p)
{
    float h = max(p.y - ubo.FogParams.z, 0.0);
    return ubo.FogParams.x * exp(-ubo.FogParams.y * h);
}

float SunVisibility(vec3 worldPos)
{
    if(ubo.SunDirection.w < 0.5)
        return 1.0;

    float viewZ  = (ubo.View * vec4(worldPos, 1.0)).z;
    int count    = int(ubo.SunColour.w);
    int cascade  = 0;
    for(int i = 0; i < MAX_SHADOWMAPS - 1; i++)
        if(i < count - 1 && viewZ < ubo.SplitDepths[i])
            cascade = i + 1;

    vec4 coord = ubo.BiasMatrix * ubo.ShadowTransform[cascade] * vec4(worldPos, 1.0);
    coord /= coord.w;
    if(coord.z <= 0.0 || coord.z >= 0.999 || any(lessThan(coord.xy, vec2(0.0))) || any(greaterThan(coord.xy, vec2(1.0))))
        return 1.0;

    float stored = textureLod(u_ShadowMap, vec3(coord.xy, float(cascade)), 0.0).r;
    return coord.z - 0.0005 > stored ? 0.0 : 1.0;
}

void main()
{
    float depth = textureLod(u_Depth, outTexCoord, 0.0).r;

    vec4 farPoint = ubo.InvViewProj * vec4(outTexCoord * 2.0 - 1.0, depth, 1.0);
    vec3 target   = farPoint.xyz / farPoint.w;
    vec3 eye      = ubo.CameraPosition.xyz;
    vec3 ray      = target - eye;
    float dist    = length(ray);
    vec3 dir      = ray / max(dist, 1e-5);
    // the sky is as far as the fog goes
    dist = depth <= 0.0 ? ubo.CameraPosition.w : min(dist, ubo.CameraPosition.w); // reversed-Z: 0 = sky

    outDepth = vec4(dist, 0.0, 0.0, 1.0);

    int steps     = int(ubo.Params.y);
    float noise   = InterleavedGradientNoise(gl_FragCoord.xy, ubo.Params.z);
    float phase   = PhaseHG(dot(dir, ubo.SunDirection.xyz), ubo.Params.x);
    vec3 sunLight = ubo.SunColour.rgb * ubo.Params.w * phase * ubo.Misc.y;

    // sky light scattered in from every direction: mostly from above
    vec3 ambient = (textureLod(u_IrradianceMap, vec3(0.0, 1.0, 0.0), 0.0).rgb * 0.7
                  + textureLod(u_IrradianceMap, vec3(0.0, -1.0, 0.0), 0.0).rgb * 0.3)
                 * ubo.Misc.x * ubo.FogParams.w;

    vec3 albedo        = ubo.FogColour.rgb;
    vec3 scattered     = vec3(0.0);
    float transmittance = 1.0;
    float prevT        = 0.0;
    for(int i = 0; i < steps; i++)
    {
        // squared spacing: more steps near the eye, where shafts are seen up close
        float s  = (float(i) + noise) / float(steps);
        float t  = s * s * dist;
        float dt = t - prevT;
        prevT    = t;

        vec3 p        = eye + dir * t;
        float density = Density(p) * ubo.FogColour.a;
        if(density < 1e-6)
            continue;

        vec3 light = sunLight * SunVisibility(p) + ambient;
        vec3 S     = light * albedo * density;

        // energy conserving step (Hillaire): integrate the in-scattering over the step
        float stepT = exp(-density * dt);
        scattered += transmittance * (S - S * stepT) / density;
        transmittance *= stepT;

        if(transmittance < 0.01)
            break;
    }

    outFog = vec4(scattered, transmittance);
}
