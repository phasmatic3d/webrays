import {OrbitPerspectiveCamera} from "./common/webgl_camera.js";
import * as webgl_viewer        from './common/webgl_viewer.js';
import * as webrays_utils       from './common/webrays_utils.js';
import * as webgl_utils         from './common/webgl_utils.js';
import * as WebRays             from './common/webrays.js';

var wr;
var gl;
var WebGLViewer;
if (!WebGLViewer) WebGLViewer = (typeof WebGLViewer !== 'undefined' ? WebGLViewer : null) || {};

WebRays.OnLoad(() => {
  rt_main();
});

//
// Main function
//
function rt_main()
{
    webgl_viewer.context_init(WebGLViewer);
    gl = WebGLViewer.gl;
    wr = webrays_utils.get_instance(gl);

    rt_init();
}

//
// Init function
//
function rt_init()
{
    WebGLViewer.camera = new OrbitPerspectiveCamera(WebGLViewer.canvas.width ,WebGLViewer.canvas.height);
    WebGLViewer.camera.attachControls(WebGLViewer.canvas);

    // Shader
    WebGLViewer.rt_shader    = {
      vertex_source:   `#version 300 es
                        precision highp float;
                        layout(location = 0) in vec3 vertex_position;
                        void main() {
                          gl_Position = vec4(vertex_position, 1.0f);
                        }`,
      fragment_source: `precision highp int;
                        precision highp float;
      
                        #define FLT_MAX   1.e27
      
                        uniform int       ads_index; 
                        uniform vec3      camera_pos, camera_up, camera_front;
                        uniform float     camera_fov;
                        uniform ivec2     dimensions;
                        
                        layout(location = 0) out vec4 rt_accumulation_OUT;

                        float NdotL(const vec3 normal_wcs, const vec3 vertex_to_light_wcs)
                        {
                          return max(0.0, dot(normal_wcs, vertex_to_light_wcs));
                        }

                        vec3 checkboard_pattern(const vec2 texcoords)
                        {
                          vec3 checkboard = vec3(1);
                          {
                            checkboard = vec3(1);
                            
                            float lines = 50.0;
                            vec2  linesWidth = vec2(2.0,2.0);
                            vec2 p = abs(fract(lines*texcoords)*2.0-1.0);
                            if(p.x < linesWidth.x / 100.0 || p.y < linesWidth.y / 100.0)
                              checkboard = vec3(0.0); 
                        
                            lines = 150.0;
                            linesWidth = vec2(2.5,2.5);
                            p = abs(fract(lines*texcoords)*2.0-1.0);
                            if(p.x < linesWidth.x / 100.0 || p.y < linesWidth.y / 100.0)
                              checkboard = vec3(0.0); 
                            
                          }
                          return checkboard;
                        }

                        void main()
                        {
                            // A. Generate Primary Rays 
                            float sx              = (gl_FragCoord.x + 0.5) / float(dimensions.x);
                            float sy              = (gl_FragCoord.y + 0.5) / float(dimensions.y);
                            float tanFOV2         = tan(camera_fov*0.5);
                            vec3  cx              = tanFOV2 * normalize(cross(camera_front, camera_up));
                            vec3  cy              = (tanFOV2 / (float(dimensions.x)/float(dimensions.y))) * normalize(cross(cx, camera_front));
                            vec3  ray_origin      = camera_pos;
                            vec3  ray_direction   = normalize((2.0*sx - 1.0)*cx + (2.0*sy - 1.0)*cy + camera_front);
                        
                            // B. Perform Ray Intersection Tests
                            ivec4 ray_intersection = wr_QueryIntersection(ads_index, ray_origin, ray_direction, FLT_MAX);
                        
                            // C. Compute Color
                            vec3  ray_color; 
                            // C.1. Miss stage
                            if (!wr_IsValidIntersection(ray_intersection)) {
                              ray_color = vec3(1.0); // White background
                            }
                            // C.2. Hit stage
                            else {

                              // Lighting data
                              vec3  light_color           = vec3(0.87, 0.87, 0.87);
                              vec3  light_position_wcs    = vec3(2.0 ,  2.0,  2.0);
                              vec3  light_direction       = vec3(-1.0, -1.0, -1.0);

                              vec3  position_wcs_v        = wr_GetInterpolatedPosition(ads_index, ray_intersection);
                              vec3  normal_wcs            = wr_GetGeomNormal          (ads_index, ray_intersection);

                              // Compute illumination
                              vec3	vertex_to_light_wcs   = light_position_wcs - position_wcs_v;
                              float	dist_to_light		      = length(vertex_to_light_wcs);
                                    vertex_to_light_wcs	  = normalize(vertex_to_light_wcs);
                            
                              float n_dot_l               = (length(normal_wcs) > 0.0) ? NdotL(normal_wcs, vertex_to_light_wcs) : 1.0;

                              // test for visibility
                              vec3  shadow_ray_origin     = position_wcs_v + 0.001 * normal_wcs;
                              vec3  shadow_ray_direction  = -light_direction;
                              float visibility            = wr_QueryOcclusion(ads_index, shadow_ray_origin, shadow_ray_direction, dist_to_light - 0.001) ? 0.0 : 1.0;
                              
                              int   materialID            = wr_GetFace(ads_index, ray_intersection).w;
                              
                              vec3 diffuse_color;
                              if(materialID == 0)
                                  diffuse_color = wr_GetInterpolatedNormal(ads_index, ray_intersection)*0.5 + 0.5;
                              else
                                  diffuse_color = vec3(0.87)*checkboard_pattern(wr_GetInterpolatedTexCoords(ads_index,ray_intersection));

                              ray_color.xyz = visibility*n_dot_l*light_color*diffuse_color;
                            }
                            rt_accumulation_OUT = vec4(ray_color, 1.0);
                        }`,
      program:         null
    }

    // Mesh containg one Cube
    WebGLViewer.mesh_cubes = {
      vertex_data:    new Float32Array ([
                                          -0.5,  0.5,  0.5,
                                           0.5,  0.5,  0.5,
                                          -0.5,  0.5, -0.5,
                                           0.5,  0.5, -0.5,
                                           0.5,  0.5,  0.5,
                                          -0.5,  0.5,  0.5,
                                           0.5, -0.5,  0.5,
                                          -0.5, -0.5,  0.5,
                                           0.5,  0.5, -0.5,
                                           0.5,  0.5,  0.5,
                                           0.5, -0.5, -0.5,
                                           0.5, -0.5,  0.5,
                                          -0.5,  0.5, -0.5,
                                           0.5,  0.5, -0.5,
                                          -0.5, -0.5, -0.5,
                                           0.5, -0.5, -0.5,
                                          -0.5,  0.5,  0.5,
                                          -0.5,  0.5, -0.5,
                                          -0.5, -0.5,  0.5,
                                          -0.5, -0.5, -0.5,
                                          -0.5, -0.5,  0.5,
                                          -0.5, -0.5, -0.5,
                                           0.5, -0.5,  0.5,
                                           0.5, -0.5, -0.5
                                        ]),
      vertex_size:    3,
      normal_data:    new Float32Array ([
                                          0, 1, 0,
                                          0, 1, 0,
                                          0, 1, 0,
                                          0, 1, 0,
                                          0, 0, 1,
                                          0, 0, 1,
                                          0, 0, 1,
                                          0, 0, 1,
                                          1, 0, 0,
                                          1, 0, 0,
                                          1, 0, 0,
                                          1, 0, 0,
                                          0, 0,-1,
                                          0, 0,-1,
                                          0, 0,-1,
                                          0, 0,-1,
                                         -1, 0, 0,
                                         -1, 0, 0,
                                         -1, 0, 0,
                                         -1, 0, 0,
                                          0,-1, 0,
                                          0,-1, 0,
                                          0,-1, 0,
                                          0,-1, 0,
                                        ]),
      normal_size:    3,
      uv_data:        new Float32Array ([]),
      uv_size:        2,
      face_data:      new Int32Array   ([
                                         0, 1,  2,  0,
                                         3, 2,  1,  0,
                                         4, 5,  6,  0,
                                         7, 6,  5,  0,
                                         8, 9,  10, 0,
                                        11, 10, 9,  0,
                                        12, 13, 14, 0,
                                        15, 14, 13, 0,
                                        16, 17, 18, 0,
                                        19, 18, 17, 0,
                                        20, 21, 22, 0,
                                        23, 22, 21, 0,
                                        ]),
      transform:      [
                      new Float32Array   ([1, 0, 0, 0,
                                           0, 1, 0, 0,
                                           0, 0, 1, 0,
                                           0, 0, 0, 1,]),
                      new Float32Array   ([1, 0, 0, 2,
                                           0, 1, 0, 0,
                                           0, 0, 1, 0,
                                           0, 0, 0, 1,]),
                      new Float32Array   ([1, 0, 0,-2,
                                           0, 1, 0, 0,
                                           0, 0, 1, 0,
                                           0, 0, 0, 1,])
                      ],
      rotateAngle:    new Int32Array([0,0,0]),
      rotateAngleStep:new Int32Array([15,5,25]),
      instance: []
    }
    
    // Mesh containg one Triangle
    WebGLViewer.mesh_planes = {
      vertex_data:    new Float32Array ([-25, -2, -25,
                                         -25, -2,  25,
                                          25, -2, -25,
                                          25, -2,  25,
                                        ]),
      vertex_size:    3,
      normal_data:    new Float32Array ([0, 1, 0,
                                         0, 1, 0,
                                         0, 1, 0,
                                         0, 1, 0]),
      normal_size:    3,
      uv_data:        new Float32Array ([0, 0,
        
                                         0, 1,
                                         1, 0,
                                         1, 1,]),
      uv_size:        2,
      face_data:      new Int32Array   ([0, 1, 2, 1,    // Different Material ID
                                         3, 2, 1, 1,]),
      transform:      [
                      new Float32Array ([1, 0, 0, 0,
                                         0, 1, 0, 0,
                                         0, 0, 1, 0,
                                        ])
                      ],
      instance:       []
    }

    WebGLViewer.timer       = webgl_utils.create_single_buffered_timer(gl);

    webgl_viewer.vao_init(WebGLViewer);
    webgl_viewer.resize(WebGLViewer);
    
    // [WR]     3. ADS setup

    // Create BLAS
    let blas_cube_index  = webrays_utils.create_blas_ads(wr);
    webrays_utils.add_shape(wr, blas_cube_index, WebGLViewer.mesh_cubes);
    let blas_plane_index = webrays_utils.create_blas_ads(wr);
    webrays_utils.add_shape(wr, blas_plane_index, WebGLViewer.mesh_planes);
    // Create TLAS
    let tlas_index       = webrays_utils.create_tlas_ads(wr);
 
    // Add 3 instances of Cube
    for (let i = 0; i < WebGLViewer.mesh_cubes.transform.length; i++) {
      WebGLViewer.mesh_cubes.instance[i] = webrays_utils.add_instance(wr, tlas_index, blas_cube_index, WebGLViewer.mesh_cubes.transform[i])
    }
    // Add 1 instance of Plane
    WebGLViewer.mesh_planes.instance[0] = webrays_utils.add_instance(wr, tlas_index, blas_plane_index, WebGLViewer.mesh_planes.transform[0]);

    WebGLViewer.ads_index = tlas_index;

    gl.disable(gl.DEPTH_TEST); 
    gl.depthMask(false);
  
    requestAnimationFrame(rt_render);
}

//
// Render function
//
function rt_render()
{
    rt_update();

    if(WebGLViewer.gl_timer_ext)
      webgl_utils.begin_single_buffered_timer(gl, WebGLViewer.gl_timer_ext, WebGLViewer.timer);
    
    rt_draw();

    if(WebGLViewer.gl_timer_ext){
      webgl_utils.end_single_buffered_timer(gl, WebGLViewer.gl_timer_ext, WebGLViewer.timer);

      const ellapsedTime = webgl_utils.get_single_buffered_timer(gl, WebGLViewer.gl_timer_ext, WebGLViewer.timer) / 1000000.0; // ms
      if(ellapsedTime !== 0.0)
        document.getElementById("timer").innerHTML = '[Timing: ' + ellapsedTime.toFixed(2) + ' ms]';
    }

    webgl_viewer.fbo_blit(WebGLViewer, WebGLViewer.framebuffer.rt_fbo);

    requestAnimationFrame(rt_render);
}

//
// Update function
//
function rt_update()
{
    // Camera update
    WebGLViewer.camera.update();

    // Update 3 instances of Cube
    for (let i = 0; i < WebGLViewer.mesh_cubes.transform.length; i++){
      let x = glMatrix.mat4.create();
      glMatrix.mat4.rotate(x, glMatrix.mat4.clone(WebGLViewer.mesh_cubes.transform[i]), glMatrix.glMatrix.toRadian(WebGLViewer.mesh_cubes.rotateAngle[i]), glMatrix.vec3.fromValues(1,0,0) );

      wr.UpdateInstance(WebGLViewer.ads_index, WebGLViewer.mesh_cubes.instance[i], x);

      WebGLViewer.mesh_cubes.rotateAngle[i] += WebGLViewer.mesh_cubes.rotateAngleStep[i];
    }
    
    let flags  = webrays_utils.update(wr);
    if (flags !== WebRays.UpdateFlags.NO_UPDATE)
    {
      let wr_fragment_source        = webrays_utils.get_shader_source(wr);
      let vertex_shader             = webgl_utils.compile_shader(gl, WebGLViewer.rt_shader.vertex_source, gl.VERTEX_SHADER);
      let fragment_source           = wr_fragment_source + WebGLViewer.rt_shader.fragment_source;
      let fragment_shader           = webgl_utils.compile_shader(gl, fragment_source, gl.FRAGMENT_SHADER);
      WebGLViewer.rt_shader.program = webgl_utils.create_program(gl, vertex_shader  , fragment_shader);
    }
}

//
// Draw function
//
function rt_draw() 
{
    gl.bindFramebuffer(gl.FRAMEBUFFER, WebGLViewer.framebuffer.rt_fbo);
    gl.viewport(0, 0, WebGLViewer.canvas.width, WebGLViewer.canvas.height);
    gl.drawBuffers([ gl.COLOR_ATTACHMENT0 ]);
 
    gl.useProgram(WebGLViewer.rt_shader.program);

    gl.uniform1i(gl.getUniformLocation(WebGLViewer.rt_shader.program, "ads_index"),
                    WebGLViewer.ads_index);
    gl.uniform2iv(gl.getUniformLocation(WebGLViewer.rt_shader.program, "dimensions"), 
                    [WebGLViewer.canvas.width, WebGLViewer.canvas.height]);
    gl.uniform3fv(gl.getUniformLocation(WebGLViewer.rt_shader.program, "camera_pos"), 
                    WebGLViewer.camera.camera_pos);
    gl.uniform3fv(gl.getUniformLocation(WebGLViewer.rt_shader.program, "camera_up"), 
                    WebGLViewer.camera.camera_up);
    gl.uniform3fv(gl.getUniformLocation(WebGLViewer.rt_shader.program, "camera_front"), 
                    WebGLViewer.camera.camera_front);
    gl.uniform1f (gl.getUniformLocation(WebGLViewer.rt_shader.program, "camera_fov"), 
                    glMatrix.glMatrix.toRadian(WebGLViewer.camera.field_of_view));

    let next_texture_unit = webrays_utils.set_bindings(wr, gl, WebGLViewer.rt_shader.program, 0);

    // Full-sceen rendering pass
    gl.bindVertexArray(WebGLViewer.vao);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bindVertexArray(null);

    while(next_texture_unit >= 0)
    {
      gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
      gl.bindTexture(gl.TEXTURE_2D, null);
      gl.bindTexture(gl.TEXTURE_2D_ARRAY, null);

      next_texture_unit = next_texture_unit - 1;
    }

    gl.useProgram(null);
}

