/* Lighting */
struct wr_light
{
  int type;
  vec3 position;
  vec3 power;
};

#define WR_LIGHT_TYPE_POINT 1
#define WR_LIGHT_TYPE_QUAD  2
wr_light Lights_sample(float u, out float pdf);
vec3 Light_sample(const in event evt, const in wr_light light, const in vec2 u, out vec3 EtL, out float pdf, out float dist);

wr_light Lights_sample(float u, out float pdf) {
  int light_index    = clamp(int(u * float(light_count)), 0, light_count - 1);
  vec4 position_type = texelFetch(lightBuffer, ivec2(0,light_index), 0);
  vec4 power_count   = texelFetch(lightBuffer, ivec2(1,light_index), 0);
  wr_light light;
  light.type = int(position_type.w);

  if (light.type == WR_LIGHT_TYPE_POINT) {
    light.position = position_type.xyz;
    light.power = power_count.xyz; 
    pdf = 1.0 / float(light_count);
  } else {
    pdf = 0.000001;
  }

  return light;
}

vec3 Light_sample(const in event evt, const in wr_light light, const in vec2 u, out vec3 EtL, out float pdf, out float dist) {
  dist = 0.0;
  pdf = 0.0;
  EtL = vec3(0);
  if (light.type == WR_LIGHT_TYPE_POINT) {
    vec3 light_direction = light.position - evt.position; 
    float light_dist = length(light_direction);
    float light_dist2 = light_dist * light_dist;
    pdf = 1.0;
    dist = light_dist;
    EtL = normalize(light_direction);
    return (light.power / WR_4PI) / max(0.001, light_dist2);
  }
  return vec3(100,100,100);
}