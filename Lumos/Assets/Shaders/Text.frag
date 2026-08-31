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
float u_threshold = 0.5;
float u_out_bias = 0.0f;
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

float pxRange = 12.0;

float ScreenPxRange()
{
    int texIdx = int(fs_in.tid - 0.5);
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(textures[texIdx], 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(fs_in.uv);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

vec4 SampleTexture(vec2 uv)
{
	vec4 texColour = vec4(0.0);

	if (fs_in.tid > 0.0)
    {
		switch(int(fs_in.tid - 0.5))
		{
			case 0: texColour = texture(textures[0], uv); break;
			case 1: texColour = texture(textures[1], uv); break;
			case 2: texColour = texture(textures[2], uv); break;
			case 3: texColour = texture(textures[3], uv); break;
			case 4: texColour = texture(textures[4], uv); break;
			case 5: texColour = texture(textures[5], uv); break;
			case 6: texColour = texture(textures[6], uv); break;
			case 7: texColour = texture(textures[7], uv); break;
			case 8: texColour = texture(textures[8], uv); break;
			case 9: texColour = texture(textures[9], uv); break;
			case 10: texColour = texture(textures[10], uv); break;
			case 11: texColour = texture(textures[11], uv); break;
			case 12: texColour = texture(textures[12], uv); break;
			case 13: texColour = texture(textures[13], uv); break;
			case 14: texColour = texture(textures[14], uv); break;
			case 15: texColour = texture(textures[15], uv); break;
		}
    }
	return texColour;
}

// Inner/outer signed distances (1.0 inside, 0.0 outside) for a given uv.
void Distances(vec2 uv, out float dInner, out float dOuter, out float dSdf)
{
    vec4 distances   = SampleTexture(uv);
    float d_msdf     = median(distances.r, distances.g, distances.b);
    dSdf             = distances.a; // mtsdf true-SDF channel
    float d_combined = mix(d_msdf, dSdf, 0.25);
    dInner = mix(d_combined, dSdf, u_rounded_fonts);
    dOuter = mix(d_combined, dSdf, u_rounded_outlines);
}

void main()
{
  // tid < 0 marks a solid quad (world-label backgrounds) - no SDF sampling.
  if (fs_in.tid < 0.0)
  {
    outColour = fs_in.colour;
    return;
  }

  float width = ScreenPxRange();
  float inverted_threshold = 1.0 - u_threshold;

  // Centre sample (used by the outline-blur / gradient features).
  float d_inner, d_outer, d_sdf;
  Distances(fs_in.uv, d_inner, d_outer, d_sdf);

  vec2 fp = fwidth(fs_in.uv);
  vec2 offs[4] = vec2[](vec2(-0.375, -0.125), vec2(0.125, -0.375),
                        vec2( 0.375,  0.125), vec2(-0.125,  0.375));

  float inner_opacity = 0.0;
  float outer_opacity = 0.0;
  for(int i = 0; i < 4; i++)
  {
      float di, do_, ds;
      Distances(fs_in.uv + offs[i] * fp, di, do_, ds);
      float inner = width * (di  - inverted_threshold) + 0.5 + u_out_bias;
      float outer = width * (do_ - inverted_threshold + u_outline_width_relative) + 0.5 + u_out_bias + u_outline_width_absolute;
      inner_opacity += smoothstep(0.0, 1.0, inner);
      outer_opacity += smoothstep(0.0, 1.0, outer);
  }
  inner_opacity *= 0.25;
  outer_opacity *= 0.25;

  vec4 inner_color = fs_in.colour;
  vec4 outer_color = fs_in.outlineColour;

  if (u_outline_blur > 0.0) {
    float blur_start = u_outline_width_relative + u_outline_width_absolute / width;
    outer_color.a = smoothstep(blur_start,
                               blur_start * (1.0 - u_outline_blur),
                               inverted_threshold - d_sdf - u_out_bias / width);
  }

  // apply some lighting (hard coded angle)
  if (u_gradient > 0.0) {
     vec2 normal = normalize(vec3(dFdx(d_inner), dFdy(d_inner), 0.01)).xy;
     float light = 0.5 * (1.0 + dot(normal, normalize(vec2(-0.3, -0.5))));
     inner_color = mix(inner_color, vec4(light, light, light, 1),
                       smoothstep(u_gradient + inverted_threshold, inverted_threshold, d_inner));
  }

  inner_opacity = pow(inner_opacity, 1.0 / u_gamma);

  vec4 color = (inner_color * inner_opacity) + (outer_color * (outer_opacity - inner_opacity));
  outColour = color;
}
