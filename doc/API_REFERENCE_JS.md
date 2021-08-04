## WebRays JavaScript API Reference

WebRays is packaged as a JavaScript ES6 `module`. Users submit a callback to the `OnLoad` function that is called when WebRays has been initialized. From there, they can use an existing `gl` context to initialize a `WebGLIntersectionEngine` object. The constructed `WebGLIntersectionEngine` object contains the bulk of the API required for adding ray tracing functionality to an existing `WebGL` application.

### WebRays Module Javascript API

|      Function          | Description     |
|:--|:--|
| OnLoad (<br />&nbsp;&nbsp;&nbsp;&nbsp;callback<br />) | Call `callback` when the WebRays module is ready to use. Only when `OnLoad` has been called, it is safe to call any WebRays specific API function |
| WebGLIntersectionEngine (<br />&nbsp;&nbsp;&nbsp;&nbsp;gl<br />) | Uses the provided `gl` context to create a WebGL-backed intersection engine. On success, it returns a JS object containing the functionality of the WebRays library. This is currently the only available backend |

### WebRays Object Javascript API

Member functions provided by a constructed `WebGLIntersectionEngine` object
```
wr = new WebRays.WebGLIntersectionEngine(gl);
```

|      Function          | Description     |
|:--|:--|
| Update () | Perform any pending updates. This function should be called after any important interaction with the WebRays API in order to submit changes. For example every time a shape is added or an instance is updated. The returned `flags` should be used by the user to determine if further updates should take place on the application side as well<br /><br /> `return`: flags indicating what has changed in the backend for the user to perform appropriate actions |
| CreateAds (<br />&nbsp;&nbsp;&nbsp;&nbsp;options<br />) | `options` is JS object with properties for the created ADS. Currently the only available option is the type of the created ADS. Simply pass a JS object with a `type` member that is either "BLAS" or "TLAS" <br /><br /> `return`: A handle for the newly created ADS that can be used to refer to this specific ADS both on the clent side and device side |
| AddShape (<br />&nbsp;&nbsp;&nbsp;&nbsp;ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;vertices,<br />&nbsp;&nbsp;&nbsp;&nbsp;vertex_stride,<br />&nbsp;&nbsp;&nbsp;&nbsp;normals,<br />&nbsp;&nbsp;&nbsp;&nbsp;normal_stride,<br />&nbsp;&nbsp;&nbsp;&nbsp;uvs,<br />&nbsp;&nbsp;&nbsp;&nbsp;uv_stride,<br />&nbsp;&nbsp;&nbsp;&nbsp;faces<br />) | Vertices, Normals and UVs are expected as `Float32Array`s. Each vertex and normal is defined by 3 consecutive `float`s (`x, y, z`). UVs are similarly defined by 2 `float`s (`u, v`). The number of attributes is expected to be the `length` of the vertex array. Faces are stored in a `Int32Array` array. They are defined by 4 consecutive `int`s (`x, y, z, w`). The first 3 are the offsets in the attribute arrays. The `w` component is left under user control amd cam be used to store per-face information <br /><br /> `return`: shape handle representing the submitted geometry group |
| AddInstance (<br />&nbsp;&nbsp;&nbsp;&nbsp;tlas,<br />&nbsp;&nbsp;&nbsp;&nbsp;blas,<br />&nbsp;&nbsp;&nbsp;&nbsp;transformation<br />) | Add an instance of an existing `blas` to an existing `tlas`. The transformation matrix is a `Float32Array`, corresponding to a 4x3 matrix in column-major order with the translation part being at positions 9, 10, and 11 <br /><br /> `return`: instance handle representing the submitted instance |
| UpdateInstance (<br />&nbsp;&nbsp;&nbsp;&nbsp;tlas,<br />&nbsp;&nbsp;&nbsp;&nbsp;instance_id,<br />&nbsp;&nbsp;&nbsp;&nbsp;transformation<br />) | Update the previously submitted instance `instance_id`. The transformation matrix is a `Float32Array`, corresponding to a 4x3 matrix in column-major order with the translation part being at positions 9, 10, and 11 |
| QueryIntersection (<br />&nbsp;&nbsp;&nbsp;&nbsp;ray_buffers,<br />&nbsp;&nbsp;&nbsp;&nbsp;isect_buffer,<br />&nbsp;&nbsp;&nbsp;&nbsp;dims<br />) | Take the rays from the provided `ray_buffers`, intersect them with the `ads` and store the **closest-hit** results in `isect_buffer` |
| QueryOcclusion (<br />&nbsp;&nbsp;&nbsp;&nbsp;ray_buffers,<br />&nbsp;&nbsp;&nbsp;&nbsp;occlusion_buffer,<br />&nbsp;&nbsp;&nbsp;&nbsp;dims<br />) | Take the rays from the provided `ray_buffers`, intersect them with the `ads` and store the binary **occlusion** results in `occlusion_buffer` |
| GetSceneAccessorString () | Returns a string representation of the accessor code. For example, in the WebGL implementation, this includes the GLSL API that can be used for in-shader intersections. `Update` flags indicate when this code has changed and users should make sure to always use the latest device-side API in their shaders. In WebGL this API simply needs to get prepended to the user's code |
| GetSceneAccessorBindings () | Get the bindings for the data structures. For example, in the WebGL implementation, these include the textures, buffers e.t.c. that are required for the GLSL API to function within a user's shader. `Update` flags indicate when these bindings have changed and users should make sure to use the latest bindings in their applciation |
| RayBufferRequirements (<br />&nbsp;&nbsp;&nbsp;&nbsp;dims<br />) | Request the requirements for a ray buffer of dimensionality specified by `dims` that will store ray origins or directions. The returned JS object will be filled with the appropriate information. For example a 2D ray buffer will naturally be backed by a 2D RGBA32F texture. The actual allocation of the buffers is left to the application for finer control. The WebGL backend currently only supports 2D ray buffers |
| IntersectionBufferRequirements (<br />&nbsp;&nbsp;&nbsp;&nbsp;dims<br />) | Request the requirements for an intersection buffer of dimensionality specified by `dims` that will receive **closest-hit** results. The returned JS object will be filled with the appropriate information. For example a 2D intersection buffer will naturally be backed by a 2D RGBA32I texture. The actual allocation of the buffers is left to the application for finer control. The WebGL backend currently only supports 2D intersection buffers |
| OcclusionBufferRequirements (<br />&nbsp;&nbsp;&nbsp;&nbsp;dims<br />) | Request the requirements for an occlusion buffer of dimensionality specified by `dims` that will receive **occlusion** results. The returned JS object will be filled with the appropriate information. For example a 2D occlusion buffer will naturally be backed by a 2D R32I texture. The actual allocation of the buffers is left to the application for finer control. The WebGL backend currently only supports 2D occlusion buffers |