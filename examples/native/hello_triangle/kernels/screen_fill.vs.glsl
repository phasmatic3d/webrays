precision highp float;

layout(location = 0) in vec3 position_IN;

void main(){
  gl_Position = vec4(position_IN, 1.0f);
}