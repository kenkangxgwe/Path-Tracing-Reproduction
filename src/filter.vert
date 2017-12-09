#version 430 core
layout(location=0) in vec3 vPosition;
layout(location=1) in vec2 texcoord;

out vec2 Texcoord;

void main() {
  gl_Position=vec4(vPosition,1.0);
  Texcoord = texcoord;
}





