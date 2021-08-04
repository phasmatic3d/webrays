import {SimplePerspectiveCamera} from "./common/webgl_camera.js";
import * as webgl_viewer         from './common/webgl_viewer.js';
import * as webrays_utils        from './common/webrays_utils.js';
import * as webgl_utils          from './common/webgl_utils.js';
import * as WebRays              from './common/webrays.js';

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
    //webgl_viewer.context_create(WebGLViewer);
    gl = WebGLViewer.gl;
    wr = webrays_utils.get_instance(gl);

    rt_init();
}

//
// Init function
//
function rt_init()
{
    // Camera
    WebGLViewer.camera       = new SimplePerspectiveCamera(WebGLViewer.canvas);

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
                              // Visualize using the barycentric coordinates of the intersection
                              ray_color.xy = wr_GetBaryCoords(ray_intersection);
                              ray_color.z  = 1.0 - ray_color.x - ray_color.y;
                            }
                            rt_accumulation_OUT = vec4(ray_color, 1.0);
                        }`,
      program:         null
    }

    // Mesh containg one Triangle
    WebGLViewer.mesh_triangle = {
      vertex_data:    new Float32Array ([-1, -1, -1,      // 1
                                          1, -1, -1,      // 2
                                          0,  1, -1]),    // 3
      vertex_size:    3,
      normal_data:    new Float32Array ([0, 0, 1,         // 1
                                         0, 0, 1,         // 2
                                         0, 0, 1]),       // 3
      normal_size:    3,
      uv_data:        new Float32Array ([0, 0,            // 1
                                         1, 0,            // 2
                                         1, 1,]),         // 3
      uv_size:        2,
      face_data:      new Int32Array   ([0, 1, 2,         // 1 indices
                                               0,])       // 1 info
    }
    
    WebGLViewer.timer = webgl_utils.create_single_buffered_timer(gl);

    webgl_viewer.vao_init(WebGLViewer);
    webgl_viewer.resize(WebGLViewer);
    
    WebGLViewer.ads_index = webrays_utils.create_blas_ads(wr);
    webrays_utils.add_shape(wr, WebGLViewer.ads_index, WebGLViewer.mesh_triangle);

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
    let flags = webrays_utils.update(wr);
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

