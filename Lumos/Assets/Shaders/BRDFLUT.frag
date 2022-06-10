#version 450
#include "Common.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//https://www.shadertoy.com/view/3lXXDB

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

// Taken from https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/genbrdflut.frag
// Based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
vec2 hammersley(uint i, uint N) 
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

// From the filament docs. Geometric Shadowing function
// https://google.github.io/filament/Filament.html#toc4.4.2
float G_Smith(float NoV, float NoL, float roughness)
{
	float k = (roughness * roughness) / 2.0;
	float GGXL = NoL / (NoL * (1.0 - k) + k);
	float GGXV = NoV / (NoV * (1.0 - k) + k);
	return GGXL * GGXV;
}

// From the filament docs. Geometric Shadowing function
// https://google.github.io/filament/Filament.html#toc4.4.2
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = pow(roughness, 4.0);
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}


// Based on Karis 2014
vec3 importanceSampleGGX(vec2 Xi, float roughness, vec3 N)
{
    float a = roughness * roughness;
    // Sample in spherical coordinates
    float Phi = 2.0 * PI * Xi.x;
    float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float SinTheta = sqrt(1.0 - CosTheta * CosTheta);
    // Construct tangent space vector
    vec3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;
    
    // Tangent to world space
    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.,0.,1.0) : vec3(1.0,0.,0.);
    vec3 TangentX = normalize(cross(UpVector, N));
    vec3 TangentY = cross(N, TangentX);
    return TangentX * H.x + TangentY * H.y + N * H.z;
}


// Karis 2014
vec2 integrateBRDF(float roughness, float NoV)
{
	vec3 V;
    V.x = sqrt(1.0 - NoV * NoV); // sin
    V.y = 0.0;
    V.z = NoV; // cos
    
    // N points straight upwards for this integration
    const vec3 N = vec3(0.0, 0.0, 1.0);
    
    float A = 0.0;
    float B = 0.0;
    const uint numSamples = 1024u;
    
    for (uint i = 0u; i < numSamples; i++) {
        vec2 Xi = hammersley(i, numSamples);
        // Sample microfacet direction
        vec3 H = importanceSampleGGX(Xi, roughness, N);
        
        // Get the light direction
        vec3 L = 2.0 * dot(V, H) * H - V;
        
        float NoL = saturate(dot(N, L));
        float NoH = saturate(dot(N, H));
        float VoH = saturate(dot(V, H));
        
        if(NoL > 0.0) {
            float V_pdf = V_SmithGGXCorrelated(NoV, NoL, roughness) * VoH * NoL / NoH;
            float Fc = pow(1.0 - VoH, 5.0);
            A += (1.0 - Fc) * V_pdf;
            B += Fc * V_pdf;
        }
    }
	
    return 4.0 * vec2(A, B) / float(numSamples);
}

layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outFrag;

void main()
{
	float a = outTexCoord.y;
    float mu = outTexCoord.x;
	
	// Output to screen
    vec2 res = integrateBRDF(a, mu);
    
    // Single Scatter Energy
    //fragColor = vec4(vec3(res.x + res.y), 1.0);
    
    // Scale and Bias for F0 (as per Karis 2014)
     outFrag = vec4(res.x, res.y, 0.0, 1.0);
}
	