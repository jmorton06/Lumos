#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 InvViewProj;
    vec4 CameraPosition; // w = max march distance
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Colour;
layout(set = 0, binding = 2) uniform sampler2D u_Fog;      // half res: rgb scattered, a transmittance
layout(set = 0, binding = 3) uniform sampler2D u_FogDepth; // half res: distance each fog texel marched
layout(set = 0, binding = 4) uniform sampler2D u_Depth;

layout(location = 0) out vec4 outFrag;

void main()
{
    vec4 scene  = textureLod(u_Colour, outTexCoord, 0.0);
    float depth = textureLod(u_Depth, outTexCoord, 0.0).r;

    vec4 world = ubo.InvViewProj * vec4(outTexCoord * 2.0 - 1.0, depth, 1.0);
    float dist = depth <= 0.0 ? ubo.CameraPosition.w : min(length(world.xyz / world.w - ubo.CameraPosition.xyz), ubo.CameraPosition.w);

    // the four fog texels round this pixel, weighted by how close their march length is to ours:
    // plain bilinear bleeds the background's fog onto a foreground edge as a halo
    vec2 lowSize = vec2(textureSize(u_Fog, 0));
    vec2 pos     = outTexCoord * lowSize - 0.5;
    vec2 base    = floor(pos);
    vec2 f       = pos - base;

    vec4 sum    = vec4(0.0);
    float total = 0.0;
    for(int i = 0; i < 4; i++)
    {
        vec2 o   = vec2(i & 1, i >> 1);
        vec2 uv  = (base + o + 0.5) / lowSize;
        float bw = (o.x > 0.5 ? f.x : 1.0 - f.x) * (o.y > 0.5 ? f.y : 1.0 - f.y);
        float d  = textureLod(u_FogDepth, uv, 0.0).r;
        float dw = 1.0 / (1e-3 + abs(d - dist) / max(dist, 1.0));
        float w  = bw * dw;
        sum += textureLod(u_Fog, uv, 0.0) * w;
        total += w;
    }
    vec4 fog = total > 0.0 ? sum / total : textureLod(u_Fog, outTexCoord, 0.0);

    outFrag = vec4(scene.rgb * fog.a + fog.rgb, scene.a);
}
