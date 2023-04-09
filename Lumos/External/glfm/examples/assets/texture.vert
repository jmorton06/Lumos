#version 100

attribute highp vec4 position;
attribute mediump vec2 texCoord;

varying mediump vec2 texCoordFragment;

void main()
{
    gl_Position = position;
    texCoordFragment = texCoord;
}
