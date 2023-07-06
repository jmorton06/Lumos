#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "EnvironmentMapping.glslh"

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform samplerCube u_Texture;

layout(push_constant) uniform PushConsts
{
	float Roughness;
	uint cubeFaceIndex;
    float p0;
    float p1;
} pushConsts;


#define PARAM_ROUGHNESS pushConsts.Roughness

// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
} 

layout(location = 0) out vec4 FragColour;

void main()
{
	vec3 N = normalize(GetCubeMapTexCoord2(int(pushConsts.cubeFaceIndex), outTexCoord));
	
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);  

    if(PARAM_ROUGHNESS < 0.06)
    {
        FragColour = vec4(texture(u_Texture, N, 0).rgb, 1.0);
    }
    else
    {
        float EnvMapSize = float(textureSize(u_Texture, 0).s);
    
        for(uint i = 0u; i < SAMPLE_COUNT; ++i)
        {
            vec2 Xi = Hammersley(i, SAMPLE_COUNT);
            vec3 H  = ImportanceSampleGGX(Xi, N, PARAM_ROUGHNESS);
            vec3 L  = normalize(2.0 * dot(V, H) * H - V);

            float NdotL = max(dot(N, L), 0.0);
            if(NdotL > 0.0)
            {
                // Vectors to evaluate pdf
                float fNdotH = saturate(dot(N, H));
                float fVdotH = saturate(dot(V, H));

                // Probability Distribution Function
                float fPdf =  D_GGX(fNdotH, PARAM_ROUGHNESS) * fNdotH / (4.0f * fVdotH);

                // Solid angle represented by this sample
                //float fOmegaS = 1.0 / (SAMPLE_COUNT * fPdf);
                float fOmegaS = 1.0 / (max(SAMPLE_COUNT * fPdf, 0.00001f));
                // Solid angle covered by 1 pixel with 6 faces that are EnvMapSize X EnvMapSize
                float fOmegaP = 4.0 * PI / (6.0 * EnvMapSize * EnvMapSize);
                // Original paper suggest biasing the mip to improve the results
                float fMipBias = 1.0f;
                float fMipLevel = max(0.5 * log2(fOmegaS / fOmegaP) + fMipBias, 0.0f);

                prefilteredColor += texture(u_Texture, L, fMipLevel).rgb * NdotL;
                totalWeight      += NdotL;
            }
        }
        prefilteredColor = prefilteredColor / max(totalWeight, 0.0001f);

        FragColour = vec4(prefilteredColor, 1.0);
    }

	
}