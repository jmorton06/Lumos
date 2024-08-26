#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 outColour;

layout (location = 0) in DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	vec4 colour;
	vec4 outlineColour;
	float outlineWidth;
} fs_in;

float u_rounded_fonts = 0.0f;
float u_rounded_outlines = 0.0f;
float u_threshold = 0.55;
float u_out_bias = 1.0f;
float u_outline_width_absolute = 0.0f;//1/16;
float u_outline_width_relative =  0.0f;//1/50;
float u_outline_blur = 2.0f;
float u_gradient = 0.0f;
float u_gamma = 1.0f;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float pxRange = 4;

float ScreenPxRange()
{
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(textures[int(fs_in.tid)], 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(fs_in.uv);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

vec4 SampleTexture()
{
	vec4 texColour = vec4(0.0);

	if (fs_in.tid > 0.0)
    {
		switch(int(fs_in.tid - 0.5))
		{
			case 0: texColour = texture(textures[0], fs_in.uv, 0); break;
			case 1: texColour = texture(textures[1], fs_in.uv); break;
			case 2: texColour = texture(textures[2], fs_in.uv); break;
			case 3: texColour = texture(textures[3], fs_in.uv); break;
			case 4: texColour = texture(textures[4], fs_in.uv); break;
			case 5: texColour = texture(textures[5], fs_in.uv); break;
			case 6: texColour = texture(textures[6], fs_in.uv); break;
			case 7: texColour = texture(textures[7], fs_in.uv); break;
			case 8: texColour = texture(textures[8], fs_in.uv); break;
			case 9: texColour = texture(textures[9], fs_in.uv); break;
			case 10: texColour = texture(textures[10], fs_in.uv); break;
			case 11: texColour = texture(textures[11], fs_in.uv); break;
			case 12: texColour = texture(textures[12], fs_in.uv); break;
			case 13: texColour = texture(textures[13], fs_in.uv); break;
			case 14: texColour = texture(textures[14], fs_in.uv); break;
			case 15: texColour = texture(textures[15], fs_in.uv); break;
		}
    }
	return texColour;

}

void main()
{
    // distances are stored with 1.0 meaning "inside" and 0.0 meaning "outside"
  vec4 distances = SampleTexture();//texture2D(u_mtsdf_font, v_texcoord);
  float d_msdf = median(distances.r, distances.g, distances.b);
  float d_sdf = distances.a; // mtsdf format only
  d_msdf = min(d_msdf, d_sdf + 0.1);  // HACK: to fix glitch in msdf near edges

  // blend between sharp and rounded corners
  float d_inner = mix(d_msdf, d_sdf, u_rounded_fonts);
  float d_outer = mix(d_msdf, d_sdf, u_rounded_outlines);

  // typically 0.5 is the threshold, >0.5 inside <0.5 outside
  float inverted_threshold = 1.0 - u_threshold; // because I want the ui to be +larger -smaller
  float width = ScreenPxRange();
  float inner = width * (d_inner - inverted_threshold) + 0.5 + u_out_bias;
  float outer = width * (d_outer - inverted_threshold + u_outline_width_relative) + 0.5 + u_out_bias + u_outline_width_absolute;

  float inner_opacity = clamp(inner, 0.0, 1.0);
  vec4 inner_color = fs_in.colour;
  float outer_opacity = clamp(outer, 0.0, 1.0);
  vec4 outer_color = fs_in.outlineColour;

  if (u_outline_blur > 0.0) {
    // NOTE: the smoothstep fails when the two edges are the same, and I wish it
    // would act like a step function instead of failing.
    // NOTE: I'm using d_sdf here because I want the shadows to be rounded
    // even when outlines are sharp. But I don't yet have implemented a way
    // to see the sharp outline with a rounded shadow.
    float blur_start = u_outline_width_relative + u_outline_width_absolute / width;
    outer_color.a = smoothstep(blur_start,
                               blur_start * (1.0 - u_outline_blur),
                               inverted_threshold - d_sdf - u_out_bias / width);
  }

  // apply some lighting (hard coded angle)
  if (u_gradient > 0.0) {
     // NOTE: this is not a no-op so it changes the rendering even when
     // u_gradient is 0.0. So I use an if() instead. But ideally I'd
     // make this do nothing when u_gradient is 0.0.
     vec2 normal = normalize(vec3(dFdx(d_inner), dFdy(d_inner), 0.01)).xy;
     float light = 0.5 * (1.0 + dot(normal, normalize(vec2(-0.3, -0.5))));
     inner_color = mix(inner_color, vec4(light, light, light, 1),
                       smoothstep(u_gradient + inverted_threshold, inverted_threshold, d_inner));
  }

  // unlike in the 2403 experiments, I do know the color is light
  // and the shadow is dark so I can implement gamma correction
  inner_opacity = pow(inner_opacity, 1.0 / u_gamma);

  vec4 color = (inner_color * inner_opacity) + (outer_color * (outer_opacity - inner_opacity));
  outColour = color;
}
