#version 430 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv_in;
uniform sampler2D texture0;
out vec2 UV;
void main(){
    UV=uv_in;
    gl_Position = vec4(position,1.0f);
}
