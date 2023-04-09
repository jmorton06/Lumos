#version 100

uniform lowp sampler2D texture0;

varying mediump vec2 texCoordFragment;

void main()
{
    gl_FragColor = texture2D(texture0, texCoordFragment);
}
