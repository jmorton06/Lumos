#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 colour;

layout (location = 0) in DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 colour;
} fs_in;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

#define GAMMA 2.2
#define DISABLE_GAMMA_CORRECTION 0

vec4 GammaCorrectTexture(vec4 samp)
{
#if DISABLE_GAMMA_CORRECTION
	return samp;
#endif
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

void main()
{
	vec4 texColor = fs_in.colour;
    if (fs_in.tid > 0.0)
    {
        
    switch(int(fs_in.tid - 0.5))
    {
			case 0: texColor *= GammaCorrectTexture(texture(textures[0], fs_in.uv)); break;
			case 1: texColor *= GammaCorrectTexture(texture(textures[1], fs_in.uv)); break;
			case 2: texColor *= GammaCorrectTexture(texture(textures[2], fs_in.uv)); break;
			case 3: texColor *= GammaCorrectTexture(texture(textures[3], fs_in.uv)); break;
			case 4: texColor *= GammaCorrectTexture(texture(textures[4], fs_in.uv)); break;
			case 5: texColor *= GammaCorrectTexture(texture(textures[5], fs_in.uv)); break;
			case 6: texColor *= GammaCorrectTexture(texture(textures[6], fs_in.uv)); break;
			case 7: texColor *= GammaCorrectTexture(texture(textures[7], fs_in.uv)); break;
			case 8: texColor *= GammaCorrectTexture(texture(textures[8], fs_in.uv)); break;
			case 9: texColor *= GammaCorrectTexture( texture(textures[9], fs_in.uv)); break;
			case 10: texColor *= GammaCorrectTexture(texture(textures[10], fs_in.uv)); break;
			case 11: texColor *= GammaCorrectTexture(texture(textures[11], fs_in.uv)); break;
			case 12: texColor *= GammaCorrectTexture(texture(textures[12], fs_in.uv)); break;
			case 13: texColor *= GammaCorrectTexture(texture(textures[13], fs_in.uv)); break;
			case 14: texColor *= GammaCorrectTexture(texture(textures[14], fs_in.uv)); break;
			case 15: texColor *= GammaCorrectTexture(texture(textures[15], fs_in.uv)); break;
//        case 16: texColor *= texture(textures[16], fs_in.uv); break;
//        case 17: texColor *= texture(textures[17], fs_in.uv); break;
//        case 18: texColor *= texture(textures[18], fs_in.uv); break;
//        case 19: texColor *= texture(textures[19], fs_in.uv); break;
//        case 20: texColor *= texture(textures[20], fs_in.uv); break;
//        case 21: texColor *= texture(textures[21], fs_in.uv); break;
//        case 22: texColor *= texture(textures[22], fs_in.uv); break;
//        case 23: texColor *= texture(textures[23], fs_in.uv); break;
//        case 24: texColor *= texture(textures[24], fs_in.uv); break;
//        case 25: texColor *= texture(textures[25], fs_in.uv); break;
//        case 26: texColor *= texture(textures[26], fs_in.uv); break;
//        case 27: texColor *= texture(textures[27], fs_in.uv); break;
//        case 28: texColor *= texture(textures[28], fs_in.uv); break;
//        case 29: texColor *= texture(textures[29], fs_in.uv); break;
//        case 30: texColor *= texture(textures[30], fs_in.uv); break;
//        case 31: texColor *= texture(textures[31], fs_in.uv); break;
    }
    }
                                    
	colour = texColor;
}
