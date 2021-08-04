#define WR_PI 3.14159265359
#define WR_2PI 6.28318530718
#define WR_INV_PI 0.31830988618
#define WR_INV_2PI 0.15915494309
#define WR_4PI 12.5663706144

struct onb
{
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
};

vec3  hemisphere_cosine_sample(vec2 u);
float hemisphere_cosine_pdf(float costheta);

onb create_onb(const in vec3 n);
vec3 onb_from(const in onb basis, vec3 v);
vec3 onb_to(const in onb basis, vec3 v);

onb create_onb(const in vec3 n)
{
  onb  basis;
  //float sign = copysignf(1.0f, n.z);
  float sign = (n.z >= 0.0) ? 1.0 : -1.0;
  float a = -1.0 / (sign + n.z);
  float b = n.x * n.y * a;
  basis.tangent = vec3(1.0 + sign * n.x * n.x * a, sign * b, -sign * n.x);
  basis.bitangent = vec3(b, sign + n.y * n.y * a, -n.y);
  basis.normal = n;
  return basis;
}

vec3
onb_from(const in onb basis, vec3 v)
{
  return normalize(v.x * basis.tangent + v.y * basis.bitangent + v.z * basis.normal);
}

vec3
onb_to(const in onb basis, vec3 v)
{
  return normalize(vec3(dot(basis.tangent, v), dot(basis.bitangent, v), dot(basis.normal, v)));
}

float hemisphere_cosine_pdf(float costheta) {
  return costheta * WR_INV_PI;
}

vec3 hemisphere_cosine_sample(vec2 u) {
  // Uniformly sample disk.
  float r = sqrt(u.x);
  float phi = 2.0 * WR_PI * u.y;
  vec3 p;
  p.x = r * cos(phi);
  p.y = r * sin(phi);

  // Project up to hemisphere.
  p.z = sqrt(max(0.0, 1.0 - p.x*p.x - p.y*p.y));
  return p;
}