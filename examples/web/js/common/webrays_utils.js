import * as WebRays from './webrays.js';

// 
// Constructor
//
export function get_instance(gl)
{
    return new WebRays.WebGLIntersectionEngine(gl);
}

// 
// Top-level Acceleration data structure creation
//
export function create_tlas_ads(wr)
{
    return wr.CreateAds({type : "TLAS" });
}

// 
// Bottom-level Acceleration data structure creation
//
export function create_blas_ads(wr)
{
    return wr.CreateAds({type : "BLAS" });
}

// 
// Fill ADS with mesh information
//
export function add_shape(wr, ads_index, mesh)
{
    wr.AddShape(ads_index, mesh.vertex_data, mesh.vertex_size,
                           mesh.normal_data, mesh.normal_size,
                           mesh.uv_data    , mesh.uv_size,
                           mesh.face_data
                );
}

// 
// Add BLAS instance to TLAS
//
export function add_instance(wr, tlas_index, blas_index, transform)
{
    return wr.AddInstance(tlas_index, blas_index, transform);
}

// 
// Update ADS
//
export function update(wr)
{
    return wr.Update();
}

// 
// Get generated shader source
//
export function get_shader_source(wr)
{
    return '#version 300 es\n' + wr.GetSceneAccessorString();
}

// 
// Map ADS bindings to the corresponding shader program
//
export function set_bindings(wr, gl, program, next_texture_unit)
{
    var bindings = wr.Bindings;
    for (var binding_index = 0; binding_index < bindings.length; ++binding_index)
    {		
      let binding = bindings[binding_index];

      // if UBO
      if (binding.Type == 1) {
      } 
      // if Texture 2D or Texture Array 2D
      else {
        let bindingType = (binding.Type == 2) ? gl.TEXTURE_2D : gl.TEXTURE_2D_ARRAY;
        
        gl.activeTexture(gl.TEXTURE0 + next_texture_unit);
        gl.bindTexture  (bindingType, binding.Texture);
        gl.uniform1i    (gl.getUniformLocation(program, binding.Name), next_texture_unit);
        next_texture_unit++;
      }
    }

    return next_texture_unit;
}