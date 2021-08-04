## WebRays GLSL API Reference

Commonly, shaders in WebGL are submitted for compilation in text-form. For convenience, WebRays also returns the entire GLSL API as a regular string which can be safely **pre**pended to the user's code. This string defines the uniforms and intersection routines of the WebRays GLSL API. Together, with the required bindings, it can be used to add intersection functionality to a WebGL application. 

|      Function          | Description     |
|:--|:--|
| `ivec4` wr_QueryIntersection (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`vec3` ray_origin,<br />&nbsp;&nbsp;&nbsp;&nbsp;`vec3` ray_direction,<br />&nbsp;&nbsp;&nbsp;&nbsp;`float` tmax<br />) | Return the closest-hit information of the intersection of the `(ray_origin, ray_direction)` ray and `ads` |
| `bool` wr_QueryOcclusion (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`vec3` ray_origin,<br />&nbsp;&nbsp;&nbsp;&nbsp;`vec3` ray_direction,<br />&nbsp;&nbsp;&nbsp;&nbsp;`float` tmax<br />) | Return occlusion information of the intersection of the `(ray_origin, ray_direction)` ray and `ads` |
| `ivec4` wr_GetFace (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get index data of the primitive at the closes hit. The `w` component directly corresponds to the `w` component that was submitted during `AddShape` |
| `vec3` wr_GetPosition (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection,<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` index<br />)  | Get 3D position at `index` |
| `vec3` wr_GetNormal (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection,<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` index<br />)  | Get 3D normal at `index` |
| `vec2` wr_GetTexCoords (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection,<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` index<br />)  | Get 2D texture coordinates at `index` |
| `vec3` wr_GetInterpolatedPosition (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get 3D position of the closest-hit |
| `vec3` wr_GetInterpolatedNormal (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get 3D normal at the closest-hit |
| `vec3` wr_GetGeomNormal (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get 3D geometric normal at the closest-hit |
| `vec2` wr_GetInterpolatedTexCoords (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get 2D texture coordinates of the closest-hit |
| `float` wr_GetHitDistance (<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get distance to closest-hit |
| `vec3` wr_GetBaryCoords3D (<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get 3D barycentric coordinates of closest hit |
| `vec2` wr_GetBaryCoords (<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Get 2D barycentric coordinates of closest hit |
| `bool` wr_IsValidIntersection (<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | Is it a **hit** (`true`) or a **miss** (`false`) |
| `int` wr_GetInstanceID (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | When `ads` is a TLAS, returns the instance id of the closest hit object |
| `mat4` wr_GetObjectTranform (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | When `ads` is a TLAS, returns the object transform matrix of the closest hit object |
| `mat4` wr_GetNormalTranform (<br />&nbsp;&nbsp;&nbsp;&nbsp;`int` ads,<br />&nbsp;&nbsp;&nbsp;&nbsp;`ivec4` intersection<br />)  | When `ads` is a TLAS, returns the normal transform matrix of the closest hit object |
