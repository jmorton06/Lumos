#version 100

attribute highp vec3 a_position;
attribute lowp vec3 a_color;

varying lowp vec3 v_color;

void main() {
    gl_Position = vec4(a_position, 1.0);
    v_color = a_color;
}
