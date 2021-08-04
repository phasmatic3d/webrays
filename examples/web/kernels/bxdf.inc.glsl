#define BxDF_DIFFUS    0x00000001u
#define Masked       0x00000002u
#define Translucent  0x00000004u

#define BxDF_DIFFUSE_REFLECTION 1
#define BxDF_SPECULAR_REFLECTION 2

struct event
{
  onb   basis;
  vec3  reflectance;
  vec3  base_color;
  vec3  shading_normal;
  vec3  position;
  int   type;
  float alpha;
};

float ggx_isotropic_ndf(const in event evt, const vec3 h);
vec3  ggx_isotropic_ndf_sample(const in event evt, const vec3 wo, const vec2 u);
float ggx_isotropic_ndf_pdf(const in event evt, const vec3 wo, const vec3 h);

vec3  fresnel_schlick(const in event evt, const float costheta);

float ggx_isotropic_lambda( const in event evt, const vec3 dir);
float ggx_isotropic_geometric(const in event evt, const vec3 wi, const vec3 wo);

float BxDF_pdfD(const in event evt, const in vec3 wo, const in vec3 wi);
float BxDF_pdfG(const in event evt, const in vec3 wo, const in vec3 wi);
vec3  BxDF_evalD(const in event evt, const in vec3 wo, const in vec3 wi);
vec3  BxDF_evalG(const in event evt, const in vec3 wo, const in vec3 wi);

// BSDF Functions
float BxDF_pdf(const in event evt, const in vec3 wo, const in vec3 wi);
vec3  BxDF_eval(const in event evt, const in vec3 wo, const in vec3 wi);
vec3  BxDF_sample(const in event evt, const in vec3 wo, const in vec2 u, out vec3 wi, out float pdf);

float BxDF_diffuse_reflection_pdf(const in event evt, const in vec3 wo, const in vec3 wi);
vec3  BxDF_diffuse_reflection_eval(const in event evt, const in vec3 wo, const in vec3 wi);
vec3  BxDF_diffuse_reflection_sample(const in event evt, const in vec3 wo, const in vec2 u, out vec3 wi, out float pdf);

vec3 BxDF_sample(const in event evt, const in vec3 woWorld, const in vec2 u, out vec3 wiWorld, out float pdf) 
{
  vec3 BxDF = vec3(0);
  vec3 wo   = onb_to(evt.basis, woWorld);
  vec3 wi;
  if (BxDF_DIFFUSE_REFLECTION == evt.type)  
    BxDF = BxDF_diffuse_reflection_sample(evt, wo, u, wi, pdf);
  else {
    wi   = vec3(0);
    BxDF = vec3(0,0,0);
  }

  wiWorld = onb_from(evt.basis, wi);

  return BxDF;  
}

vec3 BxDF_eval(const in event evt, const in vec3 woWorld, const in vec3 wiWorld)
{
  vec3 wo   = onb_to(evt.basis, woWorld);
  vec3 wi   = onb_to(evt.basis, wiWorld);
  if (BxDF_DIFFUSE_REFLECTION == evt.type)  
    return BxDF_diffuse_reflection_eval(evt, wo, wi);
  else
    return vec3(0,0,0);
}

float BxDF_pdf(const in event evt, const in vec3 woWorld, const in vec3 wiWorld)
{
  vec3 wo   = onb_to(evt.basis, woWorld);
  vec3 wi   = onb_to(evt.basis, wiWorld);
  if (BxDF_DIFFUSE_REFLECTION == evt.type)  
    return BxDF_diffuse_reflection_pdf(evt, wo, wi);
  else
    return 0.0;
}

vec3 BxDF_diffuse_reflection_sample(const in event evt, const in vec3 wo, const in vec2 u, out vec3 wi, out float pdf) {
  if ( is_zerof( dot( wo, evt.shading_normal ) ) ) 
	return vec3(0.0);

  vec3 wi_lcs = hemisphere_cosine_sample(u);
  pdf = BxDF_diffuse_reflection_pdf(evt, wo, wi_lcs);
  wi = wi_lcs;

  return BxDF_diffuse_reflection_eval(evt, wo, wi_lcs);
}

vec3 BxDF_diffuse_reflection_eval(const in event evt, const in vec3 wo, const in vec3 wi)
{
	return evt.base_color * WR_INV_PI;
}

float BxDF_diffuse_reflection_pdf(const in event evt, const in vec3 wo, const in vec3 wi)
{
  return SameHemisphere(wo, wi) ? hemisphere_cosine_pdf(AbsCosTheta(wi)) : 0.0;
}