#include <metal_stdlib>

using namespace metal;

// NOTE: Same Vertex struct in test_pattern_renderer
typedef struct {
    float2 position;
    float2 texCoord;
} VertexIn;

typedef struct {
    float4 position [[position]];
    float2 texCoord;
} VertexOut;

vertex VertexOut textureVertexShader(uint index [[vertex_id]],
                                     constant VertexIn *vertices [[buffer(0)]]) {
    VertexOut out;
    out.position = float4(vertices[index].position, 0, 1);
    out.texCoord = vertices[index].texCoord;
    return out;
}

fragment half4 textureFragmentShader(VertexOut in [[stage_in]],
                                     texture2d<half> texture [[texture(0)]],
                                     sampler textureSampler [[sampler(0)]]) {
    return texture.sample(textureSampler, in.texCoord);
}
