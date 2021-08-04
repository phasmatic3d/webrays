import * as webgl_utils from './webgl_utils.js';

//
// Initialize the GL context
//
export function context_init(webglviewer, xr)
{
  webglviewer.canvas          = document.getElementById('webrays-main-canvas');
  webglviewer.canvas.width    = webglviewer.canvas.clientWidth;
  webglviewer.canvas.height   = webglviewer.canvas.clientHeight;
  webglviewer.webgl_context_attribs = { 'majorVersion' : 2, 'minorVersion' : 0 };

  webglviewer.gl              = webglviewer.canvas.getContext("webgl2", {
    stencil:               false,
    alpha:                 false,
    antialias:             false,
    premultipliedAlpha:    false,
    preserveDrawingBuffer: false,
    depth:                 false,
    xrCompatible:          xr
  });

  const gl = webglviewer.gl;

  // If we don't have a GL context, give up now
  // Only continue if WebGL is available and working
  if (!gl) {
    throw ("Unable to initialize WebGL 2. Your browser or machine may not support it.");
  }

  var gl_ext_color_buffer_float = gl.getExtension('Ext_color_buffer_float');
  if (!gl_ext_color_buffer_float) {
    throw ("Unable to initialize WebGL ext: Ext_color_buffer_float. Your browser or machine may not support it.");
  }

  var gl_ext_float_blend = gl.getExtension('Ext_float_blend');
  if (!gl_ext_float_blend) {
    throw ("Unable to initialize WebGL ext: Ext_float_blend. Your browser or machine may not support it.");
  }

  webglviewer.gl_timer_ext = gl.getExtension('EXT_disjoint_timer_query_webgl2');
  if (!webglviewer.gl_timer_ext) {
    console.log("Unable to initialize WebGL ext: EXT_disjoint_timer_query_webgl2. Your browser or machine may not support it.");
  }
}

//
// Initialize the VAO we'll need for the sceen quad rendering
//
export function vao_init(webglviewer)
{
  const gl = webglviewer.gl;
  const screen_fill_triangle = new Float32Array(
  [ -1.0, -1.0, 0.0, 
     8.0, -1.0, 0.0, 
    -1.0,  8.0, 0.0]
  );

  const vbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
  gl.bufferData(gl.ARRAY_BUFFER, screen_fill_triangle, gl.STATIC_DRAW);
  gl.bindBuffer(gl.ARRAY_BUFFER, null);

  webglviewer.vao = gl.createVertexArray();
  gl.bindVertexArray(webglviewer.vao);
  {
    gl.bindBuffer             (gl.ARRAY_BUFFER, vbo);
    gl.vertexAttribPointer    (0, 3, gl.FLOAT, false, 0, 0); 
    gl.enableVertexAttribArray(0);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
  }
  gl.bindVertexArray(null);
  
  webgl_utils.check_error(gl);
}

//
// Initialize the framebuffers we'll need for the rendering
//
export function fbo_init(webglviewer)
{
  const gl = webglviewer.gl;
  // Delete previous state
  if(webglviewer.framebuffer !== undefined) {
    gl.deleteTexture(webglviewer.framebuffer.rt_accum_texture);
    gl.deleteFramebuffer(webglviewer.framebuffer.rt_fbo);
  }

  webglviewer.framebuffer = {
    rt_fbo:             gl.createFramebuffer(),
    rt_accum_texture:   webgl_utils.texture_2d_alloc(gl, gl.RGBA32F, webglviewer.canvas.width, webglviewer.canvas.height),
  };  

  gl.bindFramebuffer(gl.FRAMEBUFFER, webglviewer.framebuffer.rt_fbo);
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, webglviewer.framebuffer.rt_accum_texture, 0);
  webgl_utils.check_fbo_error(gl);

  gl.bindFramebuffer(gl.FRAMEBUFFER, null);
}

//
// Copy framebuffer to the screen
//
export function fbo_blit(webglviewer, read_fbo)
{
  const gl = webglviewer.gl;
  gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, null);
  gl.bindFramebuffer(gl.READ_FRAMEBUFFER, read_fbo);
  gl.readBuffer     (gl.COLOR_ATTACHMENT0);
  gl.drawBuffers    ([gl.BACK]);

  var dst_viewport = [ 0, 0, webglviewer.canvas.width, webglviewer.canvas.height];
  var src_viewport = [ 0, 0, webglviewer.canvas.width, webglviewer.canvas.height];

  gl.blitFramebuffer(
    src_viewport[0], src_viewport[1], src_viewport[2], src_viewport[3],
    dst_viewport[0], dst_viewport[1], dst_viewport[2], dst_viewport[3],
    gl.COLOR_BUFFER_BIT, gl.NEAREST);
}

//
// Resize Canvas
//
export function resize(webglviewer)
{
  webglviewer.camera.set_size(webglviewer.gl.canvas.clientWidth, webglviewer.gl.canvas.clientHeight);

  fbo_init(webglviewer);
}