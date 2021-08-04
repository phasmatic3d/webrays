precision highp float;

layout(location = 0) out vec4 out_Color;

#define FLT_MAX   1.e27

uniform int       u_ADS; 
uniform vec3      u_CameraPos, u_CameraUp, u_CameraFront;
uniform float     u_CameraFov;
uniform ivec2     u_Dimensions;

void main(){
  // A. Generate Primary Rays 
  float sx      = (gl_FragCoord.x + 0.5) / float(u_Dimensions.x);
  float sy      = (gl_FragCoord.y + 0.5) / float(u_Dimensions.y);
  float tanFOV2 = tan(radians(u_CameraFov)*0.5);
  vec3  cx      = tanFOV2 * normalize(cross(u_CameraFront, u_CameraUp));
  vec3  cy      = (tanFOV2 / (float(u_Dimensions.x)/float(u_Dimensions.y))) * normalize(cross(cx, u_CameraFront));

  vec3  ray_origin      = u_CameraPos;
  vec3  ray_direction   = normalize((2.0*sx - 1.0)*cx + (2.0*sy - 1.0)*cy + u_CameraFront);
                        
  // B. Perform Ray Intersection Tests
  ivec4 ray_intersection = wr_QueryIntersection(u_ADS, ray_origin, ray_direction, FLT_MAX);

  // C. Compute Color
  vec3  ray_color; 
  // C.1. Miss stage
  if (ray_intersection.x < 0) {
    ray_color = vec3(0,0,1.0); // White background
  }
  // C.2. Hit stage
  else {
    // Visualize using the barycentric coordinates of the intersection
    ray_color.xy = wr_GetBaryCoords(ray_intersection);
    ray_color.z  = 1.0 - ray_color.x - ray_color.y;
  }
  
  out_Color = vec4(ray_color, 0.0);
}