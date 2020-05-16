#version 430 core
in vec2 UV;
out vec4 frag_color;
uniform sampler2D texture0;
void main(){
    frag_color=vec4(texture(texture0,UV));
}
