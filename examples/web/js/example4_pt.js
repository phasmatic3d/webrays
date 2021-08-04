import * as WebRays from './common/webrays.js';
import * as webgl_utils from './common/webgl_utils.js';
import {load_gltf2, MATERIAL_TYPE} from './deps/gltf/gltf_loader.js';

WebRays.OnLoad(wrays_viewer_run);

var WebRaysViewer;
if (!WebRaysViewer) WebRaysViewer = (typeof WebRaysViewer !== 'undefined' ? WebRaysViewer : null) || {};

var wr;
var gl;
var gl_timer_ext;

const BxDF      = { DIFFUSE_REFLECTION: 1, SPECULAR_REFLECTION: 2 };
const LightType = { POINT: 1, QUAD: 2 };

function randomIntInc(low, high) {
  return Math.floor(Math.random() * (high - low + 1) + low)
}

function random_uvec2() {
  return [randomIntInc(0, 4294967295), randomIntInc(0, 4294967295)];
}

function wrays_viewer_init() {
  /* Initialize things (This could very well be a constructor) */

  WebRaysViewer.mouse           = glMatrix.vec2.fromValues(0, 0);
  WebRaysViewer.last_mouse      = glMatrix.vec2.fromValues(0, 0);
  WebRaysViewer.arcball_on      = false;

  // Viewer Properties
  WebRaysViewer.tile_width    = 256;
  WebRaysViewer.tile_height   = 256;
  WebRaysViewer.rest_depth    = 5;
  WebRaysViewer.move_depth    = 1;
  WebRaysViewer.depth         = WebRaysViewer.rest_depth;
  WebRaysViewer.frame_counter = 1;

  // Features
  WebRaysViewer.perform_accumulation = true;

  // Camera Properties
  WebRaysViewer.fov         = 60.0;
  WebRaysViewer.znear       = 0.001;
  WebRaysViewer.zfar        = 100.0;
  WebRaysViewer.camera_pos  = glMatrix.vec3.fromValues(3.0, 100.5, 6.0);

  // View Matrix
  WebRaysViewer.view = glMatrix.mat4.lookAt( 
    glMatrix.mat4.create(),
    WebRaysViewer.camera_pos, // pos
    glMatrix.vec3.fromValues( 100, 1.5, -0.4 ), // at
    glMatrix.vec3.fromValues( 0, 1, 0 ) // up
  );
  
  // Decode Camera View Axis
  WebRaysViewer.camera_right = glMatrix.vec3.fromValues( WebRaysViewer.view[0], WebRaysViewer.view[4], WebRaysViewer.view[8] );
  WebRaysViewer.camera_up    = glMatrix.vec3.fromValues( WebRaysViewer.view[1], WebRaysViewer.view[5], WebRaysViewer.view[9] );
  WebRaysViewer.camera_front = glMatrix.vec3.fromValues( -WebRaysViewer.view[2], -WebRaysViewer.view[6], -WebRaysViewer.view[10] );
  WebRaysViewer.camera_speed = 5.0;

  // FrameBuffer
  WebRaysViewer.preview_FBO = null;
  WebRaysViewer.preview_texture = null;
  WebRaysViewer.ping_pong_buffer = null;
  WebRaysViewer.final_FBO = null;
  WebRaysViewer.final_texture = null;
  WebRaysViewer.shadow_ray_FBO = null;
  WebRaysViewer.shadow_ray_texture = null;
  WebRaysViewer.ray_FBOs = [null, null];
  WebRaysViewer.ray_directions_textures = [null, null];
  WebRaysViewer.ray_origins_textures = [null, null];
  WebRaysViewer.ray_accumulation_textures = [null, null];
  WebRaysViewer.ray_payloads_textures = [null, null];
  WebRaysViewer.isect_texture = null;
  WebRaysViewer.occlusion_texture = null;

  WebRaysViewer.GBuffer = { };

  // Textures
  WebRaysViewer.env_map_texture = null;

  // Rasterization
  WebRaysViewer.fbo = null;
  WebRaysViewer.vao = null;
  WebRaysViewer.vbo = null;
  WebRaysViewer.texture = null;

  /* Program sources */
  WebRaysViewer.isect_program_fs_source = null;
  WebRaysViewer.isect_program_vs_source = null;

  WebRaysViewer.indirect_illum_program_fs_source = null;
  WebRaysViewer.indirect_illum_program_vs_source = null;

  WebRaysViewer.gbuffer_program_fs_source = null;
  WebRaysViewer.gbuffer_program_vs_source = null;

  WebRaysViewer.accum_program_fs_source = null;
  WebRaysViewer.accum_program_vs_source = null;

  /* GL Program handles */
  WebRaysViewer.pp_program = null;
  WebRaysViewer.generate_program = null;
  WebRaysViewer.nee_program = null;
  WebRaysViewer.shade_program = null;
  WebRaysViewer.isect_program = null;
  WebRaysViewer.accum_program = null;
  WebRaysViewer.indirect_illum_program = null;
  WebRaysViewer.gbuffer_program = null;
  WebRaysViewer.mesh = null;
  WebRaysViewer.gl_mesh = null;
  WebRaysViewer.textures = [];
  WebRaysViewer.lightTexture = null;
  WebRaysViewer.materialTexture = null;
  WebRaysViewer.texturesBuffer = null;
  
  WebRaysViewer.BLAS = 0;

  /* Light System */
  WebRaysViewer.lights = [];
  
  /* Resources to be loaded async */
  WebRaysViewer.shader_urls = [
                               'kernels/screen_fill.vs.glsl', 'kernels/generate.glsl',
                               'kernels/screen_fill.vs.glsl', 'kernels/intersection.glsl',
                               'kernels/screen_fill.vs.glsl', 'kernels/accumulate.glsl',
                               'kernels/screen_fill.vs.glsl', 'kernels/post_process.glsl'
                              ];
                              
  // Dcitionary of include GLSL files
  WebRaysViewer.shader_include_cache = {};

  /* Set Viewer callbacks */
  var call_during_capture = true;
  WebRaysViewer.canvas.addEventListener("mousemove", wrays_viewer_mouse_move_event, call_during_capture);
  WebRaysViewer.canvas.addEventListener("mouseup", wrays_viewer_mouse_up_event, call_during_capture);
  WebRaysViewer.canvas.addEventListener("mousedown", wrays_viewer_mouse_down_event, call_during_capture);
  WebRaysViewer.canvas.addEventListener("touchstart", wrays_viewer_touch_start_event, false);
  WebRaysViewer.canvas.addEventListener("touchend", wrays_viewer_touch_end_event, false);
  WebRaysViewer.canvas.addEventListener("touchmove", wrays_viewer_touch_move_event, false);
  window.addEventListener('keydown', wrays_viewer_key_down_event, call_during_capture);
  window.addEventListener('keyup', wrays_viewer_key_up_event, call_during_capture);

  /* Async shader load */
  Promise.all(WebRaysViewer.shader_urls.map(url =>
    fetch(url, {cache: "no-store"}).then(resp => resp.text())
  )).then(texts => {
    texts.forEach(function cacheIncludes(value, index) {
      const regexp = new RegExp(/#include\s["]([\s\S]*?)["]/g);
      const url = WebRaysViewer.shader_urls[index];
      const base = url.substring(0, url.lastIndexOf('/'));
      let match;
      while ((match = regexp.exec(value)) !== null) {
        const included = match[1];
        WebRaysViewer.shader_include_cache[included] = {source: "", base: base};
      }
    });
    const entries = Object.entries(WebRaysViewer.shader_include_cache)
    Promise.all(entries.map(included =>
      fetch(included[1].base + '/' + included[0], {cache: "no-store"}).then(resp => resp.text())
    )).then(includes => {
      includes.forEach(function cacheIncludes(value, index) {
        const filename = entries[index][0];

        WebRaysViewer.shader_include_cache[filename].source = value;
      });

      texts.forEach(function preprocessGLSL(value, index) {
        const regexp = new RegExp(/#include\s["]([\s\S]*?)["]/g);
        let match;
        let preprocessed_code = "";
        while ((match = regexp.exec(value)) !== null) {
          const included = match[1];
          const included_source = WebRaysViewer.shader_include_cache[included].source;
          const pre_code = value.substring(0, match.index);
          const post_code = value.substring(regexp.lastIndex);
          preprocessed_code += pre_code;
          preprocessed_code += included_source + '\n';
          preprocessed_code += post_code;
          value = preprocessed_code
          preprocessed_code = "";
        }
        texts[index] = value;
      });
      var vertex_shader = webgl_utils.compile_shader(gl, texts[0], gl.VERTEX_SHADER);
      var fragment_shader = webgl_utils.compile_shader(gl, texts[1], gl.FRAGMENT_SHADER);
      WebRaysViewer.generate_program = webgl_utils.create_program(gl, vertex_shader, fragment_shader);

      WebRaysViewer.isect_program_vs_source = texts[2];
      WebRaysViewer.isect_program_fs_source = texts[3];

      WebRaysViewer.accum_program_vs_source = texts[4];
      WebRaysViewer.accum_program_fs_source = texts[5];          
      
      WebRaysViewer.pp_program_vs_source = texts[6];
      WebRaysViewer.pp_program_fs_source = texts[7];

      wr = new WebRays.WebGLIntersectionEngine(gl);

      const obj = load_gltf2("https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/master/2.0/Sponza/glTF/Sponza.gltf").then(scene => {
        const images = scene.images;

        var image_array_width = 0;
        var image_array_height = 0;
        var image_array_depth = 0;
        for (const image_index in images) {
          if (scene.images[image_index].width !== 1024) 
            continue;
          image_array_width = Math.max(image_array_width, scene.images[image_index].width);
          image_array_height = Math.max(image_array_height, scene.images[image_index].height);
          image_array_depth++;
        }
       
        WebRaysViewer.array_texture = gl.createTexture();
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D_ARRAY, WebRaysViewer.array_texture);
        gl.texParameteri(gl.TEXTURE_2D_ARRAY, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D_ARRAY, gl.TEXTURE_MIN_FILTER, gl.LINEAR);

        gl.texStorage3D(gl.TEXTURE_2D_ARRAY, 1, gl.RGBA8, image_array_width, image_array_height, image_array_depth);

        for (const image_index in images) {
          if (scene.images[image_index].width !== 1024) 
            continue;
          const image = scene.images[image_index];
          gl.texSubImage3D(
            gl.TEXTURE_2D_ARRAY,
            0,
            0,
            0,
            image_index,
            image_array_width, 
            image_array_height, 
            1,
            gl.RGBA,
            gl.UNSIGNED_BYTE,
            image
          );
        }

        for (let mesh_index in scene.meshes) {
          const mesh      = scene.meshes[mesh_index].mesh;
          const images    = scene.images;
          const materials = scene.materials;

          WebRaysViewer.mesh = {};
          WebRaysViewer.mesh.faces = mesh.indices;
          WebRaysViewer.mesh.vertexNormals = mesh.normals;
          WebRaysViewer.mesh.textures = mesh.texcoords;
          WebRaysViewer.mesh.vertices  = mesh.vertices;
          WebRaysViewer.mesh.parts         = mesh.parts;

          let materials_flat = [];
          for (var material_index = 0; material_index < materials.length; material_index++) {
            const material = materials[material_index];

            materials_flat = materials_flat.concat(
              [MATERIAL_TYPE.LAMBERT,    material.baseColor[0],   material.baseColor[1],   material.baseColor[2],
              1.52, 0,   0, 0.5,
              material.baseColorTexture + 0.5,   material.MetallicRoughnessTexture  + 0.5,  material.normalsTexture + 0.5,  -1
            ]);
          }
          WebRaysViewer.materials = materials_flat;
          
          wrays_viewer_framebuffer_init();
          wrays_viewer_scene_init();

          requestAnimationFrame(wrays_viewer_render);
        }
      });
    });
  });                     
}

function wrays_viewer_run() {
  /* Canvas CSS dimension differ from WebGL viewport dimensions so we
   * make them match here
   * @see https://webglfundamentals.org/webgl/lessons/webgl-resizing-the-canvas.html
   */

  WebRaysViewer.canvas = document.getElementById('webrays-main-canvas');
  WebRaysViewer.canvas.width = WebRaysViewer.canvas.clientWidth;
  WebRaysViewer.canvas.height = WebRaysViewer.canvas.clientHeight;
  WebRaysViewer.context_handle = 0;
  WebRaysViewer.webgl_context_attribs = { 'majorVersion' : 2, 'minorVersion' : 0 };

  /* @see https://developer.mozilla.org/en-US/docs/Web/API/HTMLCanvasElement/getContext */
  WebRaysViewer.gl = gl = WebRaysViewer.canvas.getContext("webgl2", {
    stencil: false,
    alpha: false,
    antialias: false,
    premultipliedAlpha: false,
    preserveDrawingBuffer: false, /* Needed for thumbnail extraction */
    depth: false
  });

  wrays_viewer_init()
}

function wrays_viewer_resize() {
  // compute new projection matrix
  const fieldOfView = WebRaysViewer.fov * Math.PI / 180;   // in radians
  const aspectRatio = gl.canvas.clientWidth / gl.canvas.clientHeight;
  const aspect      = aspectRatio > 0 ? aspectRatio : gl.canvas.clientWidth / gl.canvas.clientHeight;

  WebRaysViewer.projection = glMatrix.mat4.perspective(glMatrix.mat4.create(),
                fieldOfView,
                aspect,
                WebRaysViewer.znear, WebRaysViewer.zfar);

  // Initialize Textures
  var buffer_info = {};

  if (gl.isFramebuffer(WebRaysViewer.preview_FBO))
    gl.deleteFramebuffer(WebRaysViewer.preview_FBO);

  if (gl.isFramebuffer(WebRaysViewer.final_FBO))
    gl.deleteFramebuffer(WebRaysViewer.final_FBO);

  if (gl.isTexture(WebRaysViewer.final_texture))
    gl.deleteTexture(WebRaysViewer.final_texture);

  WebRaysViewer.final_texture = webgl_utils.texture_2d_alloc(gl, gl.RGBA32F, gl.canvas.clientWidth, gl.canvas.clientHeight);

  if (gl.isFramebuffer(WebRaysViewer.shadow_ray_FBO))
    gl.deleteFramebuffer(WebRaysViewer.shadow_ray_FBO);

  if (gl.isFramebuffer(WebRaysViewer.ray_FBOs[0]))
    gl.deleteFramebuffer(WebRaysViewer.ray_FBOs[0]);
  if (gl.isFramebuffer(WebRaysViewer.ray_FBOs[1]))
    gl.deleteFramebuffer(WebRaysViewer.ray_FBOs[1]);

  if (gl.isTexture(WebRaysViewer.ray_directions_textures[0]))
    gl.deleteTexture(WebRaysViewer.ray_directions_textures[0]);
  if (gl.isTexture(WebRaysViewer.ray_directions_textures[1]))
    gl.deleteTexture(WebRaysViewer.ray_directions_textures[1]);

  if (gl.isTexture(WebRaysViewer.ray_origins_textures[0]))
    gl.deleteTexture(WebRaysViewer.ray_origins_textures[0]);
  if (gl.isTexture(WebRaysViewer.ray_origins_textures[1]))
    gl.deleteTexture(WebRaysViewer.ray_origins_textures[1]);

  if (gl.isTexture(WebRaysViewer.ray_accumulation_textures[0]))
    gl.deleteTexture(WebRaysViewer.ray_accumulation_textures[0]);
  if (gl.isTexture(WebRaysViewer.ray_accumulation_textures[1]))
    gl.deleteTexture(WebRaysViewer.ray_accumulation_textures[1]);

  buffer_info = wr.IntersectionBufferRequirements([WebRaysViewer.tile_width, WebRaysViewer.tile_height]);
  if (gl.TEXTURE_2D == buffer_info.Target) {
    WebRaysViewer.isect_texture = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
  }

  buffer_info = wr.RayBufferRequirements([WebRaysViewer.tile_width, WebRaysViewer.tile_height]);
  if (gl.TEXTURE_2D == buffer_info.Target) {
    WebRaysViewer.ray_accumulation_textures[0] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
    WebRaysViewer.ray_accumulation_textures[1] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
    WebRaysViewer.ray_payloads_textures[0] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
    WebRaysViewer.ray_payloads_textures[1] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
  }

  buffer_info = wr.RayDirectionBufferRequirements([WebRaysViewer.tile_width, WebRaysViewer.tile_height]);
  if (gl.TEXTURE_2D == buffer_info.Target) {
    WebRaysViewer.ray_directions_textures[0] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
    WebRaysViewer.ray_directions_textures[1] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
  }

  buffer_info = wr.RayOriginBufferRequirements([WebRaysViewer.tile_width, WebRaysViewer.tile_height]);
  if (gl.TEXTURE_2D == buffer_info.Target) {
    WebRaysViewer.ray_origins_textures[0] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
    WebRaysViewer.ray_origins_textures[1] = webgl_utils.texture_2d_alloc(gl, buffer_info.InternalFormat, buffer_info.Width, buffer_info.Height);
  }

  WebRaysViewer.ray_FBOs[0] = gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER, WebRaysViewer.ray_FBOs[0]);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, WebRaysViewer.ray_accumulation_textures[0], 0);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT1, gl.TEXTURE_2D, WebRaysViewer.ray_origins_textures[0], 0);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT2, gl.TEXTURE_2D, WebRaysViewer.ray_directions_textures[0], 0);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT3, gl.TEXTURE_2D, WebRaysViewer.ray_payloads_textures[0], 0);
  if (gl.FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(gl.FRAMEBUFFER)) {
    console.log('error FBO');
  }

  WebRaysViewer.ray_FBOs[1] = gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER, WebRaysViewer.ray_FBOs[1]);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, WebRaysViewer.ray_accumulation_textures[1], 0);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT1, gl.TEXTURE_2D, WebRaysViewer.ray_origins_textures[1], 0);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT2, gl.TEXTURE_2D, WebRaysViewer.ray_directions_textures[1], 0);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT3, gl.TEXTURE_2D, WebRaysViewer.ray_payloads_textures[1], 0);
  
  if (gl.FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(gl.FRAMEBUFFER)) {
    console.log('error FBO');
  }

  WebRaysViewer.preview_texture = webgl_utils.texture_2d_alloc(gl, gl.RGBA32F, gl.canvas.clientWidth, gl.canvas.clientHeight);

  WebRaysViewer.preview_FBO = gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER, WebRaysViewer.preview_FBO);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, WebRaysViewer.preview_texture, 0);
  if (gl.FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(gl.FRAMEBUFFER)) {
    console.log('error preview FBO');
  }

  WebRaysViewer.final_FBO = gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER, WebRaysViewer.final_FBO);
  gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, WebRaysViewer.final_texture, 0);
  if (gl.FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(gl.FRAMEBUFFER)) {
    console.log('error final FBO');
  }

  gl.bindFramebuffer(gl.FRAMEBUFFER, null);
}

function wrays_viewer_wr_scene_init() {
  WebRaysViewer.BLAS = wr.CreateAds({'type': 'BLAS'});
  const total_faces = ( WebRaysViewer.mesh.faces.length / 3 );
  const face_count  = total_faces;
  var faces = new Int32Array(face_count * 4);
  
  for (let face_index = 0; face_index < face_count; face_index++) {
    faces[face_index * 4 + 0] = WebRaysViewer.mesh.faces[face_index * 3 + 0];
    faces[face_index * 4 + 1] = WebRaysViewer.mesh.faces[face_index * 3 + 1];
    faces[face_index * 4 + 2] = WebRaysViewer.mesh.faces[face_index * 3 + 2];
    faces[face_index * 4 + 3] = 0;
  }

  for (const part_index in WebRaysViewer.mesh.parts)  {
    const part = WebRaysViewer.mesh.parts[part_index];
    for (let face_index = part.offset / 3; face_index < (part.offset + part.no_vertices) / 3; face_index++) {
      faces[face_index * 4 + 3] = part.material;
    }
  }

  wr.AddShape(WebRaysViewer.BLAS, 
          new Float32Array(WebRaysViewer.mesh.vertices), 3,
          new Float32Array(WebRaysViewer.mesh.vertexNormals), 3,
          new Float32Array(WebRaysViewer.mesh.textures), 2,
          new Int32Array(faces));
}

function wrays_viewer_gl_scene_init() {
    /* GL Programs */
  var vertex_shader = webgl_utils.compile_shader(gl, WebRaysViewer.accum_program_vs_source, gl.VERTEX_SHADER);
  var fragment_shader = webgl_utils.compile_shader(gl, WebRaysViewer.accum_program_fs_source, gl.FRAGMENT_SHADER);
  WebRaysViewer.accum_program = webgl_utils.create_program(gl, vertex_shader, fragment_shader);

  vertex_shader = webgl_utils.compile_shader(gl, WebRaysViewer.pp_program_vs_source, gl.VERTEX_SHADER);
  fragment_shader = webgl_utils.compile_shader(gl, WebRaysViewer.pp_program_fs_source, gl.FRAGMENT_SHADER);
  WebRaysViewer.pp_program = webgl_utils.create_program(gl, vertex_shader, fragment_shader);
  
  wrays_viewer_gl_scene_lights_init();
  wrays_viewer_gl_scene_materials_init();
}

function wrays_viewer_gl_scene_materials_init() {
  let materialTexture = gl.createTexture();
  let num_materials = WebRaysViewer.materials.length / 12;
  gl.bindTexture(gl.TEXTURE_2D, materialTexture);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, 3, num_materials, 0, gl.RGBA, gl.FLOAT, new Float32Array(WebRaysViewer.materials));
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.bindTexture(gl.TEXTURE_2D, null);

  WebRaysViewer.materialTexture = materialTexture;
}

function flatten(ary) {
  return ary.reduce(function(a, b) {
    if (Array.isArray(b) || ArrayBuffer.isView(b)) {
      return a.concat(flatten(b))
    }
    return a.concat(b)
  }, [])
}

function wrays_viewer_gl_scene_lights_init() {
  const light_count = WebRaysViewer.lights.length;
  for (let light_index = 0; light_index < light_count; light_index++) {
    let light = WebRaysViewer.lights[light_index];
  
    light.gl_light = {
      position: glMatrix.vec4.fromValues(light.position[0], light.position[1], light.position[2], light.type + 0.5),
      power: glMatrix.vec4.fromValues(light.power[0], light.power[1], light.power[2], 0),
      up: glMatrix.vec4.fromValues(0, 2, 0, 0),
      right: glMatrix.vec4.fromValues(2, 0, 0, 0)
    };
  }

  let gl_lights_flat = [];
  for (let light_index = 0; light_index < light_count; light_index++) {
    const light = WebRaysViewer.lights[light_index];
    const gl_light_flat = flatten(Object.values(light.gl_light));
    gl_lights_flat = gl_lights_flat.concat(gl_light_flat);
  }

  let gl_lights_flat_f32 = new Float32Array(gl_lights_flat);
  let gl_light_vec4_count = gl_lights_flat_f32.length / (light_count * 4);

  let lightTexture = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, lightTexture);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, gl_light_vec4_count, light_count, 0, gl.RGBA, gl.FLOAT,gl_lights_flat_f32);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  WebRaysViewer.lightTexture = lightTexture;

  gl.bindTexture(gl.TEXTURE_2D, null);
  gl.bindVertexArray(null);
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
  gl.bindBuffer(gl.ARRAY_BUFFER, null);
}

function wrays_viewer_scene_lights_init() {
  WebRaysViewer.lights.push({
    type: LightType.POINT,
    position: glMatrix.vec3.fromValues(500, 150.43, 0),
    power: glMatrix.vec3.fromValues(1000000, 1000000, 1000000),
  });

  WebRaysViewer.lights.push({
    type: LightType.POINT,
    position: glMatrix.vec3.fromValues(0, 150.43, 0),
    power: glMatrix.vec3.fromValues(1000000, 1000000, 1000000),
  });

  WebRaysViewer.lights.push({
    type: LightType.POINT,
    position: glMatrix.vec3.fromValues(-500, 150.43, 0),
    power: glMatrix.vec3.fromValues(1000000, 1000000, 1000000),
  });

  WebRaysViewer.lights.push({
    type: LightType.POINT,
    position: glMatrix.vec3.fromValues(900, 150.43, 0),
    power: glMatrix.vec3.fromValues(1000000, 1000000, 1000000),
  });

  WebRaysViewer.lights.push({
    type: LightType.POINT,
    position: glMatrix.vec3.fromValues(-900, 150.43, 0),
    power: glMatrix.vec3.fromValues(1000000, 1000000, 1000000),
  });
}

function wrays_viewer_scene_init() {
  wrays_viewer_scene_lights_init();

  wrays_viewer_wr_scene_init();
  wrays_viewer_gl_scene_init();
}

function wrays_viewer_framebuffer_init() {
  WebRaysViewer.vao = gl.createVertexArray();
  gl.bindVertexArray(WebRaysViewer.vao);
  var screen_fill_triangle = new Float32Array(
                          [ -4.0, -4.0, 0.0,
                             4.0, -4.0, 0.0, 
                             0.0, 4.0, 0.0 ]
                          );
  
  WebRaysViewer.vbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, WebRaysViewer.vbo);
  gl.bufferData(gl.ARRAY_BUFFER, screen_fill_triangle, gl.STATIC_DRAW);  
  gl.enableVertexAttribArray(0);
  gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0); 
 
  wrays_viewer_resize();

  gl.bindVertexArray(null);
  gl.bindBuffer(gl.ARRAY_BUFFER, null);
  gl.bindFramebuffer(gl.FRAMEBUFFER, null);
  gl.bindTexture(gl.TEXTURE_2D, null);
}

function wrays_viewer_key_up_event(key) {
  const code = key.keyCode;
  switch (code) {
    case 65:  glMatrix.vec3.sub(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_right, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.rest_depth; break;
    case 87:  glMatrix.vec3.add(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_front, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.rest_depth; break;
    case 83:  glMatrix.vec3.sub(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_front, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.rest_depth; break;
    case 68:  glMatrix.vec3.add(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_right, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.rest_depth; break;
    case 37:  break; // Left key
    case 38:  break; // Up key
    case 39:  break; // Right key
    case 40:  break; // Down key

    default:  break; // Everything else
  }
}

function wrays_viewer_key_down_event(key) {
  const code = key.keyCode;
  switch (code) {
    case 65:  glMatrix.vec3.sub(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_right, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.move_depth; break;
    case 87:  glMatrix.vec3.add(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_front, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.move_depth; break;
    case 83:  glMatrix.vec3.sub(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_front, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.move_depth; break;
    case 68:  glMatrix.vec3.add(WebRaysViewer.camera_pos, WebRaysViewer.camera_pos, glMatrix.vec3.scale(glMatrix.vec3.create(), WebRaysViewer.camera_right, WebRaysViewer.camera_speed)); WebRaysViewer.frame_counter = 0; WebRaysViewer.depth = WebRaysViewer.move_depth; break;
    case 37:  break; // Left key
    case 38:  break; // Up key
    case 39:  break; // Right key
    case 40:  break; // Down key

    default:  break; // Everything else
  }

  const at = glMatrix.vec3.add(glMatrix.vec3.create(), WebRaysViewer.camera_front, WebRaysViewer.camera_pos);
  WebRaysViewer.view = glMatrix.mat4.lookAt( 
    glMatrix.mat4.create(),
    WebRaysViewer.camera_pos, // pos
    at, // at
    WebRaysViewer.camera_up // up
  ); // view is [right, up, forward, -pos]^T;
}

function wrays_viewer_touch_start_event(touch) {
  WebRaysViewer.frame_counter = 0;
  WebRaysViewer.arcball_on = true;
  WebRaysViewer.mouse = WebRaysViewer.last_mouse = glMatrix.vec2.fromValues(touch.touches[0].clientX, touch.touches[0].clientY);
}

function wrays_viewer_touch_end_event(touch) {
  WebRaysViewer.arcball_on = false;
}

function wrays_viewer_touch_move_event(touch) {
  WebRaysViewer.frame_counter = 0;
  if (WebRaysViewer.arcball_on) {
    WebRaysViewer.depth = WebRaysViewer.move_depth;
    WebRaysViewer.mouse = glMatrix.vec2.fromValues(touch.touches[0].clientX, touch.touches[0].clientY);
	}
}
  
function wrays_viewer_mouse_move_event(mouse) {
  if (WebRaysViewer.arcball_on) {
    WebRaysViewer.frame_counter = 0;
    WebRaysViewer.depth = WebRaysViewer.move_depth;
    WebRaysViewer.mouse = glMatrix.vec2.fromValues(mouse.offsetX, mouse.offsetY);
	}
}

function wrays_viewer_mouse_up_event(mouse) {
  switch( mouse.button ) {
    case 0:
      WebRaysViewer.arcball_on = false;
      WebRaysViewer.depth = WebRaysViewer.rest_depth;
      break;
    case 1:
      break;
    case 2:
      break;  
    default:
      break;
  }
}

function wrays_viewer_mouse_down_event(mouse) {
  switch( mouse.button ) {
    case 0:
      WebRaysViewer.arcball_on = true;
			WebRaysViewer.mouse = WebRaysViewer.last_mouse = glMatrix.vec2.fromValues(mouse.offsetX, mouse.offsetY);
      break;
    case 1:
      break;
    case 2:
      break;
    default:
      break;
  }
}

function wrays_viewer_camera_update() {
  if (WebRaysViewer.mouse[0] == WebRaysViewer.last_mouse[0] && 
      WebRaysViewer.mouse[1] == WebRaysViewer.last_mouse[1])
    return;
    
  var delta = glMatrix.vec2.fromValues(1, 1);
  const mouse_delta = glMatrix.vec2.sub(glMatrix.vec2.create(), WebRaysViewer.last_mouse, WebRaysViewer.mouse);
  delta = glMatrix.vec2.mul(glMatrix.vec2.create(), delta, mouse_delta);
  delta[0] *= 0.004; // NEED to multiply with dt
  delta[1] *= 0.002;// NEED to multiply with dt

  WebRaysViewer.last_mouse[0] = WebRaysViewer.mouse[0];
  WebRaysViewer.last_mouse[1] = WebRaysViewer.mouse[1];
        
  let camera_at = glMatrix.vec3.clone(WebRaysViewer.camera_front);
  let rot_x = glMatrix.mat4.fromRotation(glMatrix.mat4.create(), delta[0], WebRaysViewer.camera_up);
  let rot_y = glMatrix.mat4.fromRotation(glMatrix.mat4.create(), delta[1], WebRaysViewer.camera_right);
  let rot = glMatrix.mat4.multiply(glMatrix.mat4.create(), rot_x, rot_y);
  
  camera_at = glMatrix.vec3.transformMat4(glMatrix.vec3.create(), camera_at, rot);
    
  camera_at = glMatrix.vec3.add(glMatrix.vec3.create(), camera_at, WebRaysViewer.camera_pos);
  WebRaysViewer.view = glMatrix.mat4.lookAt( 
    glMatrix.mat4.create(),
    WebRaysViewer.camera_pos, // pos
    camera_at, // at
    WebRaysViewer.camera_up // up
  ); // view is [right, up, -forward, -pos]^T;

  WebRaysViewer.camera_right = glMatrix.vec3.fromValues( WebRaysViewer.view[0], WebRaysViewer.view[4], WebRaysViewer.view[8] );
  WebRaysViewer.camera_up = glMatrix.vec3.fromValues( WebRaysViewer.view[1], WebRaysViewer.view[5], WebRaysViewer.view[9] );
  WebRaysViewer.camera_front = glMatrix.vec3.fromValues( -WebRaysViewer.view[2], -WebRaysViewer.view[6], -WebRaysViewer.view[10] );
}

function wrays_viewer_update() {
  wrays_viewer_camera_update();

  const update_flags = wr.Update();
  if(WebRays.UpdateFlags.NO_UPDATE == update_flags)
    return;

  var scene_accessor_string = wr.GetSceneAccessorString();
  var fragment_source = '#version 300 es\n' + scene_accessor_string + WebRaysViewer.isect_program_fs_source;
  var vertex_shader = webgl_utils.compile_shader(gl, WebRaysViewer.isect_program_vs_source, gl.VERTEX_SHADER);
  var fragment_shader = webgl_utils.compile_shader(gl, fragment_source, gl.FRAGMENT_SHADER);
  WebRaysViewer.isect_program = webgl_utils.create_program(gl, vertex_shader, fragment_shader);
}

function wrays_viewer_post_process(texture) {
  gl.bindFramebuffer(gl.FRAMEBUFFER, null);
  gl.viewport(0, 0, WebRaysViewer.canvas.width, WebRaysViewer.canvas.height);

  gl.useProgram(WebRaysViewer.pp_program);

  let index = gl.getUniformLocation(WebRaysViewer.pp_program, "accumulated_texture");
  gl.activeTexture(gl.TEXTURE0 + 0);
	gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.uniform1i(index, 0);
  index = gl.getUniformLocation(WebRaysViewer.pp_program, "positions_texture");
  gl.activeTexture(gl.TEXTURE0 + 1);
	gl.bindTexture(gl.TEXTURE_2D, WebRaysViewer.GBuffer.positions_texture);
  gl.uniform1i(index, 1);
  index = gl.getUniformLocation(WebRaysViewer.pp_program, "normals_texture");
  gl.activeTexture(gl.TEXTURE0 + 2);
	gl.bindTexture(gl.TEXTURE_2D, WebRaysViewer.GBuffer.normals_texture);
  gl.uniform1i(index, 2);
  index = gl.getUniformLocation(WebRaysViewer.pp_program, "color_texture");
  gl.activeTexture(gl.TEXTURE0 + 3);
	gl.bindTexture(gl.TEXTURE_2D, WebRaysViewer.GBuffer.color_texture);
  gl.uniform1i(index, 3);

  gl.drawArrays(gl.TRIANGLES, 0, 3);
}

function wrays_viewer_accumulate(tile_index_x, tile_index_y, texture) {
  gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, WebRaysViewer.preview_FBO);

  gl.viewport(0, 0, WebRaysViewer.canvas.width, WebRaysViewer.canvas.height);
      
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
  gl.useProgram(WebRaysViewer.accum_program);

  let index = gl.getUniformLocation(WebRaysViewer.accum_program, "blend_factor");
  gl.uniform1f(index, (WebRaysViewer.perform_accumulation) ? 1.0 / WebRaysViewer.frame_counter : 1.0);
  index = gl.getUniformLocation(WebRaysViewer.accum_program, "accumulation_texture");
  gl.activeTexture(gl.TEXTURE0 + 0);
	gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.uniform1i(index, 0);

  gl.drawArrays(gl.TRIANGLES, 0, 3);
  gl.disable(gl.BLEND);
}

function wrays_viewer_blit(tile_index_x, tile_index_y, src_fbo) {
  gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, WebRaysViewer.final_FBO);
  gl.bindFramebuffer(gl.READ_FRAMEBUFFER, src_fbo);

  gl.readBuffer(gl.COLOR_ATTACHMENT0);

  var dst_viewport = [
    (tile_index_x + 0) * WebRaysViewer.tile_width,
    (tile_index_y + 0) * WebRaysViewer.tile_height,
    Math.min((tile_index_x + 1) * WebRaysViewer.tile_width, WebRaysViewer.canvas.width),
    Math.min((tile_index_y + 1) * WebRaysViewer.tile_height, WebRaysViewer.canvas.height)
  ];
  var src_viewport = [
    0,0,
    Math.min(WebRaysViewer.tile_width, WebRaysViewer.tile_width + WebRaysViewer.canvas.width - (tile_index_x + 1) * WebRaysViewer.tile_width),
    Math.min(WebRaysViewer.tile_height, WebRaysViewer.tile_height + WebRaysViewer.canvas.height - (tile_index_y + 1) * WebRaysViewer.tile_height)
  ];

  gl.blitFramebuffer(
    src_viewport[0], src_viewport[1], src_viewport[2], src_viewport[3],
    dst_viewport[0], dst_viewport[1], dst_viewport[2], dst_viewport[3],
    gl.COLOR_BUFFER_BIT, gl.NEAREST);
}

function wrays_viewer_isects_handle(tile_index_x, tile_index_y, depth) {
  const ray_directions    = WebRaysViewer.ray_directions_textures[(depth + 0) & 1];
  const ray_accumulations = WebRaysViewer.ray_accumulation_textures[(depth + 0) & 1];
  const ray_payloads      = WebRaysViewer.ray_payloads_textures[(depth + 0) & 1];
  const ray_origins       = WebRaysViewer.ray_origins_textures[(depth + 0) & 1];
  const ray_fbo           = WebRaysViewer.ray_FBOs[(depth + 1) & 1];
  const ray_intersections = WebRaysViewer.isect_texture;
   
  gl.bindFramebuffer(gl.FRAMEBUFFER, ray_fbo);
  gl.drawBuffers([ gl.COLOR_ATTACHMENT0, gl.COLOR_ATTACHMENT1, gl.COLOR_ATTACHMENT2, gl.COLOR_ATTACHMENT3 ]);

  gl.viewport(0, 0, WebRaysViewer.tile_width, WebRaysViewer.tile_height);
      
  gl.useProgram(WebRaysViewer.isect_program);
 
  var index = gl.getUniformLocation(WebRaysViewer.isect_program, "tile");
  gl.uniform4iv(index, [tile_index_x, tile_index_y, WebRaysViewer.tile_width, WebRaysViewer.tile_height]);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "frame");
  gl.uniform2iv(index, [WebRaysViewer.canvas.width, WebRaysViewer.canvas.height]);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "seed");
  gl.uniform2uiv(index, random_uvec2());
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "sample_index");
  gl.uniform1i(index,  WebRaysViewer.frame_counter);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "depth");
  gl.uniform1i(index,  depth);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "ads");
  gl.uniform1i(index, WebRaysViewer.BLAS);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "light_count");
  gl.uniform1i(index, WebRaysViewer.lights.length);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "ray_directions");
  gl.activeTexture(gl.TEXTURE0 + 0);
	gl.bindTexture(gl.TEXTURE_2D, ray_directions);
  gl.uniform1i(index, 0);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "ray_origins");
  gl.activeTexture(gl.TEXTURE0 + 1);
	gl.bindTexture(gl.TEXTURE_2D, ray_origins);
  gl.uniform1i(index, 1);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "env_map");
  gl.activeTexture(gl.TEXTURE0 + 2);
	gl.bindTexture(gl.TEXTURE_2D, WebRaysViewer.env_map_texture);
  gl.uniform1i(index, 2);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "intersections");
  gl.activeTexture(gl.TEXTURE0 + 3);
	gl.bindTexture(gl.TEXTURE_2D, ray_intersections);
  gl.uniform1i(index, 3);
  // Material Properties
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "materialBuffer");
  gl.activeTexture(gl.TEXTURE0 + 4);
  gl.bindTexture(gl.TEXTURE_2D, WebRaysViewer.materialTexture);
  gl.uniform1i(index, 4);
  // Object Textures
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "texturesBuffer");
  gl.activeTexture(gl.TEXTURE0 + 5);
  //gl.bindTexture(gl.TEXTURE_2D_ARRAY, WebRaysViewer.texturesBuffer);
  gl.bindTexture(gl.TEXTURE_2D_ARRAY, WebRaysViewer.array_texture);
  gl.uniform1i(index, 5);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "lightBuffer");
  gl.activeTexture(gl.TEXTURE0 + 6);
  gl.bindTexture(gl.TEXTURE_2D, WebRaysViewer.lightTexture);
  gl.uniform1i(index, 6);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "ray_accumulations");
  gl.activeTexture(gl.TEXTURE0 + 7);
  gl.bindTexture(gl.TEXTURE_2D, ray_accumulations);
  gl.uniform1i(index, 7);
  index = gl.getUniformLocation(WebRaysViewer.isect_program, "ray_payloads");
  gl.activeTexture(gl.TEXTURE0 + 8);
  gl.bindTexture(gl.TEXTURE_2D, ray_payloads);
  gl.uniform1i(index, 8);
  var bindings = wr.Bindings;

  var next_texture_unit = 9;
  for (var binding_index = 0; binding_index < bindings.length; ++binding_index) {		
    var binding = bindings[binding_index];

    /* if UBO */
    if (binding.Type == WebRays.BindingType.GL_UNIFORM_BLOCK) {
      
    /* if Texture 2D */
    } else if (binding.Type == WebRays.BindingType.GL_TEXTURE_2D) {
      index = gl.getUniformLocation(WebRaysViewer.isect_program, binding.Name);
      gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
      gl.bindTexture(gl.TEXTURE_2D, binding.Texture);
      gl.uniform1i(index, next_texture_unit);
      next_texture_unit++;
    /* if Texture Array 2D */
    } else if (binding.Type == WebRays.BindingType.GL_TEXTURE_2D_ARRAY ) {
      index = gl.getUniformLocation(WebRaysViewer.isect_program, binding.Name);
      gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
      gl.bindTexture(gl.TEXTURE_2D_ARRAY, binding.Texture);
      gl.uniform1i(index, next_texture_unit);
      next_texture_unit++;
    }
  }

  gl.drawArrays(gl.TRIANGLES, 0, 3);
}

function wrays_viewer_rays_generate(tile_index_x, tile_index_y, fbo) {
  gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
  
  gl.drawBuffers([ gl.COLOR_ATTACHMENT0, gl.COLOR_ATTACHMENT1, gl.COLOR_ATTACHMENT2, gl.COLOR_ATTACHMENT3 ]);
  gl.viewport(0, 0, WebRaysViewer.tile_width, WebRaysViewer.tile_height);

  var projectionInvMatrix = glMatrix.mat4.create();
  glMatrix.mat4.invert(projectionInvMatrix, WebRaysViewer.projection);
  var viewInvMatrix = glMatrix.mat4.create();
  glMatrix.mat4.invert(viewInvMatrix, WebRaysViewer.view);
  
  gl.useProgram(WebRaysViewer.generate_program);

  var index = gl.getUniformLocation(WebRaysViewer.generate_program, "tile");
  gl.uniform4iv(index, [tile_index_x, tile_index_y, WebRaysViewer.tile_width, WebRaysViewer.tile_height]);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "frame");
  gl.uniform2iv(index, [WebRaysViewer.canvas.width, WebRaysViewer.canvas.height]);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "seed");
  gl.uniform2uiv(index, random_uvec2());
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "camera_pos");
  gl.uniform3fv(index, WebRaysViewer.camera_pos);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "camera_up");
  gl.uniform3fv(index, WebRaysViewer.camera_up);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "camera_front");
  gl.uniform3fv(index, WebRaysViewer.camera_front);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "camera_right");
  gl.uniform3fv(index, WebRaysViewer.camera_right);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "ProjectionInvMatrix");
  gl.uniformMatrix4fv(index, false, projectionInvMatrix);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "ViewInvMatrix");
  gl.uniformMatrix4fv(index, false, viewInvMatrix);
  index = gl.getUniformLocation(WebRaysViewer.generate_program, "camera_vfov");
  gl.uniform1f(index, WebRaysViewer.fov);

  gl.drawArrays(gl.TRIANGLES, 0, 3);
}

function wrays_viewer_render_rt(now) {
  var tile_count_x = ((WebRaysViewer.canvas.width - 1) / WebRaysViewer.tile_width) + 1;
	var tile_count_y = ((WebRaysViewer.canvas.height - 1) / WebRaysViewer.tile_height) + 1;
 
  gl.bindVertexArray(WebRaysViewer.vao);

  WebRaysViewer.frame_counter += 1;
  
  const rays = {directions: WebRaysViewer.ray_directions_textures, origins: WebRaysViewer.ray_origins_textures};
  const isects = WebRaysViewer.isect_texture;
  const targets = WebRaysViewer.ray_FBOs;
  let target = null;
  for (var tile_index_x = 0; tile_index_x < tile_count_x; tile_index_x++) {
		for (var tile_index_y = 0; tile_index_y < tile_count_y; tile_index_y++) {
      target = targets[0];

      wrays_viewer_rays_generate(tile_index_x, tile_index_y, target);

      for (var depth = 0; depth < WebRaysViewer.depth; depth++  ) {
        target = targets[(depth + 1) & 1];
        const directions    = rays.directions[depth & 1];
        const origins       = rays.origins[depth & 1];
        const intersections = isects;
         
        wr.QueryIntersection([origins, directions], intersections, [WebRaysViewer.tile_width, WebRaysViewer.tile_height]);

        wrays_viewer_isects_handle(tile_index_x, tile_index_y, depth);
      }

      wrays_viewer_blit(tile_index_x, tile_index_y, target);
    }
  }

  wrays_viewer_accumulate(tile_index_x, tile_index_y, WebRaysViewer.final_texture);
  wrays_viewer_post_process(WebRaysViewer.preview_texture);
}

function wrays_viewer_render(now) {
  wrays_viewer_update();

  wrays_viewer_render_rt();

  gl.bindVertexArray(null);
  gl.useProgram(null);

  requestAnimationFrame(wrays_viewer_render);
}
