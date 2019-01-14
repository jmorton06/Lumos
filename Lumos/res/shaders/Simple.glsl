#shader vertex

uniform mat4 sys_ProjectionMatrix;
uniform mat4 sys_ViewMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

out vec3 fragColor;
out vec2 fragTexCoord;

void main() 
{
    gl_Position = sys_ProjectionMatrix * sys_ViewMatrix * modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
	fragTexCoord = inTexCoord;
}
#shader end

#shader fragment

in vec3 fragColor;
in vec2 fragTexCoord;

uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec4 texColour = texture(texSampler, fragTexCoord);
	if(texColour.w < 0.4)
		discard;

    outColor = texColour; // * vec4(fragColor, 1.0f);
}
#shader end
