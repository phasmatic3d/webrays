#version 300 es
precision highp float;
precision highp int;

layout(location = 0) out vec4 pixel_color_OUT;

uniform sampler2D accumulated_texture;

void main() {
  vec4 pixel_color = texelFetch(accumulated_texture, ivec2(gl_FragCoord.xy), 0);
  pixel_color_OUT = pow(pixel_color, vec4(1.0 / 2.2));
}
