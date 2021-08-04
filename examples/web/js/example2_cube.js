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
                        uniform float     frame_count;
                        uniform uvec2     seed;
                        uniform vec3      camera_pos, camera_up, camera_front;
                        uniform float     camera_fov;

                        uniform ivec2     dimensions;
                        
                        layout(location = 0) out vec4 rt_accumulation_OUT;

                        /* compute the upper 32 bits of the product of two unsigned 32-bit integers */
                        void umulExtended_(uint a, uint b, out uint hi, out uint lo) {
                            const uint WHALF = 16u;
                            const uint LOMASK = (1u<<WHALF)-1u;
                            lo = a*b;               /* full low multiply */
                            uint ahi = a>>WHALF;
                            uint alo = a& LOMASK;
                            uint bhi = b>>WHALF;
                            uint blo = b& LOMASK;
                        
                            uint ahbl = ahi*blo;
                            uint albh = alo*bhi;
                        
                            uint ahbl_albh = ((ahbl&LOMASK) + (albh&LOMASK));
                            hi = ahi*bhi + (ahbl>>WHALF) +  (albh>>WHALF);
                            hi += ahbl_albh >> WHALF; /* carry from the sum of lo(ahbl) + lo(albh) ) */
                            /* carry from the sum with alo*blo */
                            hi += ((lo >> WHALF) < (ahbl_albh&LOMASK)) ? 1u : 0u;
                        }

                        uvec2 
                        philox4x32Bumpkey(uvec2 key) {
                          uvec2 ret = key;
                          ret.x += 0x9E3779B9u;
                          ret.y += 0xBB67AE85u;
                          return ret;
                        }
                      
                        uvec4
                        philox4x32Round(uvec4 state, uvec2 key) {
                          const uint M0 = 0xD2511F53u, M1 = 0xCD9E8D57u;
                          uint hi0, lo0, hi1, lo1;
                          umulExtended_(M0, state.x, hi0, lo0);
                          umulExtended_(M1, state.z, hi1, lo1);
                      
                          return uvec4(
                              hi1^state.y^key.x, lo1,
                              hi0^state.w^key.y, lo0);
                        }

                        float uintToFloat(uint src) {
                          return uintBitsToFloat(0x3f800000u | (src & 0x7fffffu))-1.0;
                        }
                      
                        vec4 uintToFloat(uvec4 src) {
                          return vec4(uintToFloat(src.x), uintToFloat(src.y), uintToFloat(src.z), uintToFloat(src.w));
                        }

                        vec4
                        _random(uint index, uint seed0, uint seed1) {
                          
                          uvec2 key = uvec2( index , seed0 );
                          uvec4 ctr = uvec4( 0u , 0xf00dcafeu, 0xdeadbeefu, seed1 ); 
                        
                          uvec4 state = ctr;
                          uvec2 round_key = key;
                        
                          for(int i=0; i<7; ++i) {
                            state     = philox4x32Round(state, round_key);
                            round_key = philox4x32Bumpkey(round_key);
                          }
                        
                          return uintToFloat(state);
                        }

                        void main()
                        {
                            uint  counter = uint(gl_FragCoord.x) + uint(gl_FragCoord.y) * uint(dimensions.x) + 1u;
                            vec4  randoms = _random(counter, seed.x, seed.y);

                            // A. Generate Primary Rays 
                            float sx              = (gl_FragCoord.x + randoms.x) / float(dimensions.x);
                            float sy              = (gl_FragCoord.y + randoms.y) / float(dimensions.y);
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
                              // Visualize using the interpolated normal coordinates of the intersection
                              ray_color.xyz = wr_GetInterpolatedNormal(ads_index, ray_intersection)*0.5 + 0.5;
                            }
                            rt_accumulation_OUT = vec4(ray_color, 1.0 / frame_count);
                        }`,
      program:         null
    }

    // Mesh containg one Cube
    WebGLViewer.mesh_cube = {
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
                                        ])
    }
    
    WebGLViewer.frame_count = 0;
    WebGLViewer.timer = webgl_utils.create_single_buffered_timer(gl);

    webgl_viewer.vao_init(WebGLViewer);
    webgl_viewer.resize(WebGLViewer);
    
    WebGLViewer.ads_index = webrays_utils.create_blas_ads(wr, WebGLViewer.mesh_cube);
    webrays_utils.add_shape(wr, WebGLViewer.ads_index, WebGLViewer.mesh_cube);

    gl.disable(gl.DEPTH_TEST); 
    gl.depthMask(false);
  
    gl.enable(gl.BLEND);
    gl.blendEquation(gl.FUNC_ADD);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA); 

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
    if(WebGLViewer.camera.update())
      WebGLViewer.frame_count = 0;

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
    WebGLViewer.frame_count++;

    gl.bindFramebuffer(gl.FRAMEBUFFER, WebGLViewer.framebuffer.rt_fbo);
    gl.viewport(0, 0, WebGLViewer.canvas.width, WebGLViewer.canvas.height);
    gl.drawBuffers([ gl.COLOR_ATTACHMENT0 ]);
 
    gl.useProgram(WebGLViewer.rt_shader.program);

    gl.uniform1i(gl.getUniformLocation(WebGLViewer.rt_shader.program, "ads_index"),
                    WebGLViewer.ads_index);
    gl.uniform1f(gl.getUniformLocation(WebGLViewer.rt_shader.program, "frame_count"),
                    WebGLViewer.frame_count);
    gl.uniform2ui(gl.getUniformLocation(WebGLViewer.rt_shader.program, "seed"), 
                    Math.floor(Math.random() * 10000), Math.floor(Math.random() * 10000));
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

