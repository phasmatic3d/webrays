// BSDF Inline Functions
float CosTheta(in vec3 w) { return w.z; }
float Cos2Theta(in vec3 w) { return w.z * w.z; }
float AbsCosTheta(in vec3 w) { return abs(w.z); }
bool  hemisphere_same(vec3 a, vec3 b, vec3 n);
bool  SameHemisphere(vec3 a, vec3 b) { return a.z * b.z > 0.0; }

bool hemisphere_same(vec3 a, vec3 b, vec3 n) {
  return (dot(a, n) * dot(b, n) > 0.0);
}