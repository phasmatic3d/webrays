/* Copyright 2021 Phasmatic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export var WebRaysModule = {};

class WebRaysException extends Error {
  constructor(...params) {
    // Pass remaining arguments (including vendor specific ones) to parent constructor
    super(...params)

    // Maintains proper stack trace for where our error was thrown (only available on V8)
    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, WebRaysException)
    }

    this.name = 'WebRaysError'
  }
}

function wrays_pointer_word_size() { 
  return 1;
}
function wrays_free(addr) { 
  return WebRaysModule._free(addr);
}

function wrays_alloc_ints(count) { 
  return WebRaysModule._malloc(Int32Array.BYTES_PER_ELEMENT * count);
}
function wrays_create_ints(addr, count) { 
  const int_heap = new Int32Array(WebRaysModule.HEAPU8.buffer, addr, count);
  return int_heap;
}
function wrays_alloc_int() { 
  return WebRaysModule._malloc(Int32Array.BYTES_PER_ELEMENT);
}
function wrays_create_int(addr) { 
  const int_heap = new Int32Array(WebRaysModule.HEAPU8.buffer, addr, Int32Array.BYTES_PER_ELEMENT);
  return int_heap[0];
}

function wrays_alloc_uints(count) { 
  return WebRaysModule._malloc(Uint32Array.BYTES_PER_ELEMENT * count);
}
function wrays_create_uints(addr, count) { 
  const int_heap = new Uint32Array(WebRaysModule.HEAPU8.buffer, addr, Uint32Array.BYTES_PER_ELEMENT * count);
  return int_heap;
}
function wrays_alloc_uint() { 
  return WebRaysModule._malloc(Uint32Array.BYTES_PER_ELEMENT);
}
function wrays_create_uint(addr) { 
  const uint_heap = new Uint32Array(WebRaysModule.HEAPU8.buffer, addr, Uint32Array.BYTES_PER_ELEMENT);
  return uint_heap[0];
}

function wrays_create_string(addr, max_count) { 
  return WebRaysModule.UTF8ToString(addr, max_count);
}
function wrays_string_to_heap(cseq){
  const numBytes = WebRaysModule.lengthBytesUTF8(cseq) + 1;
  const ptr = WebRaysModule._malloc(numBytes);

  WebRaysModule.stringToUTF8(cseq, ptr, numBytes);

  return ptr;
}
function wrays_array_to_heap(typedArray){
  const numBytes = typedArray.length * typedArray.BYTES_PER_ELEMENT;
  const ptr = WebRaysModule._malloc(numBytes);
  const heapBytes = new Uint8Array(WebRaysModule.HEAPU8.buffer, ptr, numBytes);
  heapBytes.set(new Uint8Array(typedArray.buffer));
  return ptr;
}
function wrays_count_options(options) {
  return Object.keys(options).length;
}
function wrays_flatten_options(options) {
  const options_flat = Object.keys(options).reduce(function (r, k) {
    return r.concat(k, String(options[k]));
  }, []);
  const options_count = options_flat.length / 2;
  const options_byte_count = 2 * options_count * wrays_pointer_word_size() * Uint32Array.BYTES_PER_ELEMENT;
  const options_pairs = new Uint32Array(options_count * 2);
  for (var i = 0; i < options_count * 2; i+=2) {
    options_pairs[i + 0] = wrays_string_to_heap(options_flat[i + 0]);
    options_pairs[i + 1] = wrays_string_to_heap(options_flat[i + 1]);
  }
  
  return wrays_array_to_heap(options_pairs);
}

function wrays_free_options(options_pair_ptr, options_count) {
  const options_pairs = wrays_create_uints(options_pair_ptr, options_count);
  for (let i = 0; i < options_count * 2; i++) {
    wrays_free(options_pairs[i]);
  }
  wrays_free(options_pair_ptr);
}
function wrays_flag(x) {
  return (1 << (x))
}

  export const BackendType = { 
    NONE: 0, 
    GLES: 1, 
    GL: 2, 
    VULKAN: 3, 
    WEBGPU: 4, 
    CPU: 5
  };

  export const BindingType = { 
    NONE: 0, 
    GL_UNIFORM_BLOCK: 1, 
    GL_TEXTURE_2D: 2, 
    GL_TEXTURE_2D_ARRAY: 3, 
    GL_STORAGE_BUFFER: 4, 
    CPU_BUFFER: 5
  };

  export const UpdateFlags = { 
    NO_UPDATE : 0, 
    ACCESSOR_BINDINGS : wrays_flag(0),
    ACCESSOR_CODE : wrays_flag(1),
    INSTANCE_UPDATE : wrays_flag(2),
  };

  export function OnLoad(callback)
  {
    createWebRaysModule().then(instance => {
      WebRaysModule = instance;
      
      /*WebRaysModule.print = function(text) { 
        console.log(text);
      };
  
      WebRaysModule.locateFile = function(path,scriptDirectory) {
        return "./js/common/" + path;
      };*/
      
      callback();
    });
  }

  export function WebGLIntersectionEngine(gl) {

    const float_texture_ext = gl.getExtension('EXT_color_buffer_float');
    if (null == float_texture_ext) {
      throw new WebRaysException("The current version of WebRays requires the EXT_color_buffer_float extention");
    }

    let major_ptr = wrays_alloc_int();
    let minor_ptr = wrays_alloc_int();

    WebRaysModule['_wrays_version'](major_ptr, minor_ptr);

    this.GLContextAttribs = { 'majorVersion' : 2, 'minorVersion' : 0 };
    this.GL = WebRaysModule.GL;
    this.GLCanvas = gl.canvas;
    this.GLContext = WebRaysModule.GL.createContext(gl.canvas, this.GLContextAttribs);
    this.GL.makeContextCurrent(this.GLContext);
    
    this.Backend = BackendType.GLES;
    this.BackendDescription = "WebGL 2";
    this.Version = {
      major: wrays_create_int(major_ptr),
      minor: wrays_create_int(minor_ptr),
      description: wrays_create_string(WebRaysModule['_wrays_version_string'](), 256)
    };
    wrays_free(major_ptr);
    wrays_free(minor_ptr);

    this.VAO = gl.createVertexArray();
    gl.bindVertexArray(this.VAO);
    const screen_fill_triangle = new Float32Array(
                            [ -4.0, -4.0, 0.0,
                               4.0, -4.0, 0.0, 
                               0.0, 4.0, 0.0 ]
                            );
    
    this.VBO = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.VBO);
    gl.bufferData(gl.ARRAY_BUFFER, screen_fill_triangle, gl.STATIC_DRAW);  
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0); 

    gl.bindVertexArray(null);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);

    /* Get callbacks to native functions */
    this.Context = WebRaysModule['_wrays_init'](this.Backend);
    this.IsectProgram = null;
    this.OcclusionProgram = null;
    this.OcclusionBuffers = [];
    this.IsectBuffers = [];
    this.Bindings = [];
    this.BindingPtr = wrays_alloc_int(5);
    this.BufferInfoPtr = wrays_alloc_int(5);
    this.DimensionsPtr = wrays_alloc_uint(16);
    this.RayBufferRequirements = this.RayOriginBufferRequirements = this.RayDirectionBufferRequirements = function(dims) {
      let buffer_info = {};
      if (!Array.isArray(dims))
        return buffer_info;

      if (dims.length === 2) {
        let dims_ptr = wrays_array_to_heap(new Uint32Array([dims[0], dims[1]]));
        WebRaysModule['_wrays_ray_buffer_requirements'](this.Context, this.BufferInfoPtr, dims_ptr, 2);
        const buffer_info_ints = wrays_create_ints(this.BufferInfoPtr, 5);
        wrays_free(dims_ptr);

        const type = buffer_info_ints[0];
        if(type === 2) {
          buffer_info = {
            Type : buffer_info_ints[0],
            Target : buffer_info_ints[1],
            InternalFormat : buffer_info_ints[2],
            Width : buffer_info_ints[3],
            Height : buffer_info_ints[4]
          };
        } else {

        }
      } else if (dims.length === 1) {
        const count = dims[0];
      }
      
      return buffer_info;
    };

    this.IntersectionBufferRequirements = function(dims) {
      let buffer_info = {};
      if (!Array.isArray(dims))
        return buffer_info;

      if (dims.length === 2) {
        let dims_ptr = wrays_array_to_heap(new Uint32Array([dims[0], dims[1]]));
        WebRaysModule['_wrays_intersection_buffer_requirements'](this.Context, this.BufferInfoPtr, dims_ptr, 2);
        let buffer_info_ints = wrays_create_ints(this.BufferInfoPtr, 5);
        wrays_free(dims_ptr);
        
        let type = buffer_info_ints[0];
        if(type === 2) {
          buffer_info = {
            Type : buffer_info_ints[0],
            Target : buffer_info_ints[1],
            InternalFormat : buffer_info_ints[2],
            Width : buffer_info_ints[3],
            Height : buffer_info_ints[4]
          };
        } else {

        }
      } else if (dims.length === 1) {
        const count = dims[0];
      }
      
      return buffer_info;
    };
    this.OcclusionBufferRequirements = function(dims) {
      var buffer_info = {};
      if (!Array.isArray(dims))
        return buffer_info;

      if (dims.length == 2) {
        var dims_ptr = wrays_array_to_heap(new Uint32Array([dims[0], dims[1]]));
        WebRaysModule['_wrays_occlusion_buffer_requirements'](this.Context, this.BufferInfoPtr, dims_ptr, 2);
        var buffer_info_ints = wrays_create_ints(this.BufferInfoPtr, 5);
        wrays_free(dims_ptr);
        
        const type = buffer_info_ints[0];
        if(2 == type) {
          buffer_info = {
            Type : buffer_info_ints[0],
            Target : buffer_info_ints[1],
            InternalFormat : buffer_info_ints[2],
            Width : buffer_info_ints[3],
            Height : buffer_info_ints[4]
          };
        } else {

        }
      } else if (dims.length == 1) {
        const count = dims[0];
      }
      
      return buffer_info;
    };
    this.GetSceneAccessorString = function() {
      var accessor_ptr = WebRaysModule['_wrays_get_scene_accessor'](this.Context);
      return wrays_create_string(accessor_ptr, 100000);
    };
    this.GetSceneAccessorBindings = function() {
      const int_count = wrays_pointer_word_size() + 1 + wrays_pointer_word_size() + 1;
      const byte_count = int_count * 4;
      var binding_count_ptr = wrays_alloc_int();
      var bindings_ptr = WebRaysModule['_wrays_get_scene_accessor_bindings'](this.Context, binding_count_ptr);
      let bindings = [];

      const binding_count = wrays_create_int(binding_count_ptr);
      for (let binding_index = 0; binding_index < binding_count; ++binding_index) {
        const bindings_ints = wrays_create_ints(bindings_ptr + byte_count * binding_index, int_count);
        const binding_type = bindings_ints[1];
        let binding = {};
        if (binding_type === BindingType.GL_UNIFORM_BLOCK) {
          binding = {
            Name : wrays_create_string(bindings_ints[0], 256),
            Type : binding_type,
            UBO  : this.GL.buffers[bindings_ints[2]]
          };
        } else if (binding_type === BindingType.GL_TEXTURE_2D) {
          binding = {
            Name : wrays_create_string(bindings_ints[0], 256),
            Type : binding_type,
            Texture  : this.GL.textures[bindings_ints[2]]
          };
        } else if (binding_type === BindingType.GL_TEXTURE_2D_ARRAY) {
          binding = {
            Name : wrays_create_string(bindings_ints[0], 256),
            Type : binding_type,
            Texture  : this.GL.textures[bindings_ints[2]]
          };
        }

        bindings.push(binding);
      }
      wrays_free(binding_count_ptr);

      return bindings;
    };
    this.Update = function() {
      let update_flags_ptr = wrays_alloc_int();
      const error = WebRaysModule['_wrays_update'](this.Context, update_flags_ptr);
      const update_flags = wrays_create_int(update_flags_ptr);
      wrays_free(update_flags_ptr);

      if(error !== 0)
      {
        const error_msg = wrays_create_string(error, 256);
        throw new WebRaysException("Error in updating the intersection engine: " + error_msg);
      }

      if (update_flags === 0)
        return update_flags;

      // Get the intersection and occlusion program handles
      let isect_program_ptr = wrays_alloc_uint();
      let occlusion_program_ptr = wrays_alloc_uint();     

      WebRaysModule['__wrays_internal_get_intersection_kernel'](this.Context, isect_program_ptr);
      WebRaysModule['__wrays_internal_get_occlusion_kernel'](this.Context, occlusion_program_ptr);
     
      const isect_program = wrays_create_uint(isect_program_ptr);
      const occlusion_program = wrays_create_uint(occlusion_program_ptr);
      
      this.IsectProgram = this.GL.programs[isect_program];
      this.OcclusionProgram = this.GL.programs[occlusion_program];
      this.Bindings = this.GetSceneAccessorBindings();

      wrays_free(isect_program_ptr);
      wrays_free(occlusion_program_ptr);

      return update_flags;
    };
    this.QueryOcclusion = function(ray_buffers, occlusion_buffer, dims) {
      if (!Array.isArray(dims) || !Array.isArray(ray_buffers)) {
        throw new WebRaysException("An array is expected for ray buffers and dimentions");
      }
    
      const gl = WebRaysModule.GL.currentContext.GLctx;
      let width = 0;
      let height = 0;
      let fbo = null;

      if (occlusion_buffer.hasOwnProperty('WebRaysData')) { 
        fbo = occlusion_buffer.WebRaysData.FBO;
        width = occlusion_buffer.WebRaysData.Width;
        height = occlusion_buffer.WebRaysData.Height;
      }
      if (null === fbo) {
        fbo = gl.createFramebuffer();
        gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
        gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, occlusion_buffer, 0);
        if (gl.FRAMEBUFFER_COMPLETE !== gl.checkFramebufferStatus(gl.FRAMEBUFFER)) {
          throw new WebRaysException("Internal WebRays Error: failed to create Occlusion Buffer FBO");
        }
        this.OcclusionBuffers.push({ Handle : occlusion_buffer, FBO : fbo, Width : dims[0], Height : dims[1] });
        occlusion_buffer.WebRaysData = { Handle : occlusion_buffer, FBO : fbo, Width : dims[0], Height : dims[1] };
        width = dims[0];
        height = dims[1];
      }

      gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);

      const current_vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
      gl.bindVertexArray(this.VAO);
	    gl.drawBuffers([gl.COLOR_ATTACHMENT0]);
      gl.viewport(0, 0, width, height);
      
      gl.useProgram(this.OcclusionProgram);
      let index = gl.getUniformLocation(this.OcclusionProgram, "wr_RayOrigins");
	    gl.activeTexture(gl.TEXTURE0 + 0);
	    gl.bindTexture(gl.TEXTURE_2D, ray_buffers[0]);
	    gl.uniform1i(index, 0);
	    index = gl.getUniformLocation(this.OcclusionProgram, "wr_RayDirections");
	    gl.activeTexture(gl.TEXTURE0 + 1);
	    gl.bindTexture(gl.TEXTURE_2D, ray_buffers[1]);
	    gl.uniform1i(index, 1);
      index = gl.getUniformLocation(this.OcclusionProgram, "wr_ADS");
      gl.uniform1i(index, 0);
      
      const bindings = this.Bindings;
      let next_texture_unit = 2;
      for (let binding_index = 0; binding_index < bindings.length; ++binding_index) {		
        const binding = bindings[binding_index];
    
        /* if UBO */
        if (binding.Type == BindingType.GL_UNIFORM_BLOCK) {
          console.error("Binding type of UBO is not implemented yet.");
          return "Wrong binding type";
        /* if Texture 2D */
        } else if (binding.Type == BindingType.GL_TEXTURE_2D) {
          index = gl.getUniformLocation(this.OcclusionProgram, binding.Name);
          gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
          gl.bindTexture(gl.TEXTURE_2D, binding.Texture);
          gl.uniform1i(index, next_texture_unit);
          next_texture_unit++;
        /* if Texture Array 2D */
        } else if (binding.Type == BindingType.GL_TEXTURE_2D_ARRAY) {
          index = gl.getUniformLocation(this.OcclusionProgram, binding.Name);
          gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
          gl.bindTexture(gl.TEXTURE_2D_ARRAY, binding.Texture);
          gl.uniform1i(index, next_texture_unit);
          next_texture_unit++;
        }
      }

      gl.drawArrays(gl.TRIANGLES, 0, 3);
      
      /* Rethink */
      gl.bindVertexArray(current_vao);
      gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    };
    this.QueryIntersection = function(ray_buffers, isect_buffer, dims) {
      if (!Array.isArray(dims) || !Array.isArray(ray_buffers)) {
        throw new WebRaysException("An array is expected for intersection buffers and dimentions");
      }

      const gl = WebRaysModule.GL.currentContext.GLctx;
      let width = 0;
      let height = 0;
      let fbo = null;

      if (isect_buffer.hasOwnProperty('WebRaysData')) { 
        fbo = isect_buffer.WebRaysData.FBO;
        width = isect_buffer.WebRaysData.Width;
        height = isect_buffer.WebRaysData.Height;
      }
      if (null == fbo) {
        fbo = gl.createFramebuffer();
        gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
        gl.framebufferTexture2D( gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, isect_buffer, 0);
        if (gl.FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(gl.FRAMEBUFFER)) {
          throw new WebRaysException("Internal WebRays Error: failed to create Intersection Buffer FBO");
        }
        this.IsectBuffers.push({ Handle : isect_buffer, FBO : fbo, Width : dims[0], Height : dims[1] });
        isect_buffer.WebRaysData = { Handle : isect_buffer, FBO : fbo, Width : dims[0], Height : dims[1] };
        width = dims[0];
        height = dims[1];
      }

      gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);

      let current_vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
      gl.bindVertexArray(this.VAO);
	    gl.drawBuffers([gl.COLOR_ATTACHMENT0]);
      gl.viewport(0, 0, width, height);
      
      gl.useProgram(this.IsectProgram);
      let index = gl.getUniformLocation(this.IsectProgram, "wr_RayOrigins");
	    gl.activeTexture(gl.TEXTURE0 + 0);
	    gl.bindTexture(gl.TEXTURE_2D, ray_buffers[0]);
	    gl.uniform1i(index, 0);
	    index = gl.getUniformLocation(this.IsectProgram, "wr_RayDirections");
	    gl.activeTexture(gl.TEXTURE0 + 1);
	    gl.bindTexture(gl.TEXTURE_2D, ray_buffers[1]);
	    gl.uniform1i(index, 1);
      index = gl.getUniformLocation(this.IsectProgram, "wr_ADS");
      gl.uniform1i(index, 0);
      
      let bindings = this.Bindings;
      let next_texture_unit = 2;
      for (let binding_index = 0; binding_index < bindings.length; ++binding_index) {		
        let binding = bindings[binding_index];
    
        /* if UBO */
        if (binding.Type == BindingType.GL_UNIFORM_BLOCK) {
          
        /* if Texture 2D */
        } else if (binding.Type == BindingType.GL_TEXTURE_2D) {
          index = gl.getUniformLocation(this.IsectProgram, binding.Name);
          gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
          gl.bindTexture(gl.TEXTURE_2D, binding.Texture);
          gl.uniform1i(index, next_texture_unit);
          next_texture_unit++;
        /* if Texture Array 2D */
        } else if (binding.Type == BindingType.GL_TEXTURE_2D_ARRAY) {
          index = gl.getUniformLocation(this.IsectProgram, binding.Name);
          gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
          gl.bindTexture(gl.TEXTURE_2D_ARRAY, binding.Texture);
          gl.uniform1i(index, next_texture_unit);
          next_texture_unit++;
        }
      }

      gl.drawArrays(gl.TRIANGLES, 0, 3);
      
      /* Rethink */
      gl.bindVertexArray(current_vao);
      gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    };
    this.OcclusionBufferDestroy = function(buffer) {
    };
    this.RayBufferDestroy = function(buffer) {
    };
    this.IntersectionBufferDestroy = function(buffer) {
    };
    this.CreateAds = function(options) {
      let ads_id_ptr = wrays_alloc_int();
      const options_count = wrays_count_options(options)
      const options_ptr = wrays_flatten_options(options)

      const error = WebRaysModule['_wrays_create_ads'](this.Context, ads_id_ptr, options_ptr, options_count);
      let ads_id = wrays_create_int(ads_id_ptr);

      wrays_free_options(options_ptr, options_count);
      wrays_free(ads_id_ptr);

      if(error !== 0)
      {
        const error_msg = wrays_create_string(error, 256);
        throw new WebRaysException("Error in creating an ADS: " + error_msg);
      }

      return ads_id;
    };
    this.AddShape = function(ads, vertices, vertex_stride, normals, normal_stride, uvs, uv_stride, faces) {
      if ( vertices === null )
      {
        throw new WebRaysException("Vertex buffer should not be null");
      }

      vertex_stride = (vertex_stride === 0) ? 3 : vertex_stride;
      normal_stride = (normal_stride === 0) ? 3 : normal_stride;
      uv_stride     = (uv_stride     === 0) ? 2 : uv_stride;

      const attr_count = vertices.length / vertex_stride;
      const face_count = faces.length / 4;
      
      const vertices_ptr = ( attr_count > 0 ) ? wrays_array_to_heap(vertices) : 0;
      const normals_ptr  = ( null != normals && normals.length > 0 ) ? wrays_array_to_heap(normals) : 0;
      const uvs_ptr      = ( null != uvs && uvs.length > 0 ) ? wrays_array_to_heap(uvs) : 0;
      const faces_ptr    = ( face_count > 0 ) ? wrays_array_to_heap(faces) : 0;
      const shape_id_ptr = wrays_alloc_int();
      const error = WebRaysModule['_wrays_add_shape'](this.Context, ads, vertices_ptr, vertex_stride, normals_ptr, normal_stride, uvs_ptr, uv_stride, attr_count, faces_ptr, face_count, shape_id_ptr);
      const shape_id = wrays_create_int(shape_id_ptr);
      wrays_free(shape_id_ptr);

      if(error !== 0)
      {
        const error_msg = wrays_create_string(error, 256);
        throw new WebRaysException("Error in adding a shape: " + error_msg);
      }
      
      if ( 0 != vertices_ptr) wrays_free(vertices_ptr);
      if ( 0 != normals_ptr)  wrays_free(normals_ptr);
      if ( 0 != uvs_ptr)      wrays_free(uvs_ptr);
      if ( 0 != faces_ptr)    wrays_free(faces_ptr);

      return shape_id;
    };
    this.AddInstance = function(tlas, blas, transform) {
      if(Array.isArray(transform))
            transform = new Float32Array(transform);
      if (!(transform instanceof Float32Array))
      {
        throw new WebRaysException("Transformation matrix should be provided");
      }
      if(transform.length < 12)
      {
        throw new WebRaysException("Transformation matrix should be provided (4x3)");
      }

      const instance_id_ptr = wrays_alloc_int();      
      const transformation_ptr = wrays_array_to_heap(transform);

      const error = WebRaysModule['_wrays_add_instance'](this.Context, tlas, blas, transformation_ptr, instance_id_ptr);
      const instance_id = wrays_create_int(instance_id_ptr);
      wrays_free(instance_id_ptr);

      if(error !== 0)
      {
        const error_msg = wrays_create_string(error, 256);
        throw new WebRaysException("Error in adding an instance: " + error_msg);
      }
      
     
      if ( 0 != transformation_ptr) wrays_free(transformation_ptr);

      return instance_id;
    };
    this.UpdateInstance = function(tlas, instance_id, transform) {
      if(Array.isArray(transform))
        transform = new Float32Array(transform);
      if (!(transform instanceof Float32Array)) {
        throw new WebRaysException("Transformation matrix should be provided");
      }
      if(transform.length < 12)
      {
        throw new WebRaysException("Transformation matrix should be provided (4x3)");
      }

      const transformation_ptr = wrays_array_to_heap(transform);

      const error = WebRaysModule['_wrays_update_instance'](this.Context, tlas, instance_id, transformation_ptr);
      if(error !== 0)
      {
        const error_msg = wrays_create_string(error, 256);
        throw new WebRaysException("Error in adding an instance: " + error_msg);
      }
      
      if ( 0 != transformation_ptr) wrays_free(transformation_ptr);

      return instance_id;
    };
  }
