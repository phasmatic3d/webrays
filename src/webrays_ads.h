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

#ifndef _WRAYS_ACCELERATION_DATA_STRUCTURE_H_
#define _WRAYS_ACCELERATION_DATA_STRUCTURE_H_

#include "webrays_math.h"
#include "webrays_context.h"

#include <webrays/webrays.h>

#include <vector>
#include <limits>

struct wr_bounds
{
  vec3 min{ std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max() };
  vec3 max{ std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest() };
};

struct wr_linear_bvh_node
{
  wr_bounds bounds;
  union
  {
    int primitivesOffset;  // leaf
    int secondChildOffset; // interior
  };
  uint16_t nPrimitives; // 0 -> interior node
  uint8_t  axis;        // interior node: xyz
  uint8_t  pad[1];      // ensure 32 byte total size
};

#define WR_MAX_BINDINGS 8

class ADS
{

public:
  const wr_binding*
  GetBindings()
  {
    return m_webgl_bindings;
  }
  int
  GetBindingsLength()
  {
    return m_webgl_binding_count;
  }

  virtual int
  AddTriangularMesh(float* positions, int position_stride, float* normals,
                    int normal_stride, float* uvs, int uv_stride,
                    int num_vertices, int* indices, int num_indices) = 0;
  virtual int
  AddSphere(float* position, float radius, int materialID) = 0;

  virtual bool
  Build() = 0;

  virtual const char*
  GetIntersectionCode() = 0;

  std::vector<vec4>  m_normal_data;
  std::vector<vec4>  m_vertex_data;
  std::vector<ivec4> m_triangles; // Triangles (v0, v1, v2, ID)

  wr_binding m_webgl_bindings[WR_MAX_BINDINGS];
  int        m_webgl_binding_count;
  int        m_vertex_texture_size;
  int        m_index_texture_size;
  int        m_instance_texture_size;
};

class SAHBVH : public ADS
{

public:
  struct Textures
  {
    unsigned int scene_vertices; // 2D array (posXYZ, UV.x) | (normXYZ, UV.y)
    unsigned int scene_indices;  // (v0,v1,v2, ID)
    unsigned int
      bvh_nodes; // 2D (bbox_min.xyz, prim_child_offset), (bbox_max.xyz, (nPrim
                 // (16bit), axis(8 bit), padd(8 bit)))
    unsigned int attributes[8]; // Attributes buffer
  };

  struct TriangleMesh
  {
    int ID; // ShapeID
    int vertex_offset;
    int num_vertices;
    int triangle_offset;
    int num_triangles;
  };

  std::vector<TriangleMesh> m_triangle_meshes;

  const int           maxPrimsInNode;
  int                 m_total_nodes;
  wr_linear_bvh_node* m_linear_nodes;

  int m_shape_id_generator;
  int m_material_id_generator;
  int m_node_texture_size;

  bool m_need_update;

  Textures m_webgl_textures;

  char* intersection_code;

  SAHBVH();
  ~SAHBVH();

  int
  AddTriangularMesh(float* positions, int position_stride, float* normals,
                    int normal_stride, float* uvs, int uv_stride,
                    int num_vertices, int* indices,
                    int num_indices) final override;
  int
  AddSphere(float* position, float radius, int materialID) final override;

  bool
  Build() final override;

  const char*
  GetIntersectionCode() final override;
  const Textures
  GetBVHTexture()
  {
    return m_webgl_textures;
  }
};

class LinearNodes : public ADS
{

public:
  struct Textures
  {
    unsigned int scene_vertices; // 2D array (posXYZ, UV.x) | (normXYZ, UV.y)
    unsigned int scene_indices;  // (v0,v1,v2, ID)
    unsigned int attributes[8];  // Attributes buffer
  };

private:
  struct TriangleMesh
  {
    int ID; // ShapeID
    int vertex_offset;
    int num_vertices;
    int triangle_offset;
    int num_triangles;
  };

  std::vector<TriangleMesh> m_triangle_meshes;

  int m_shape_id_generator;
  int m_material_id_generator;

  bool m_need_update;

  Textures m_webgl_textures;

  char* intersection_code;

public:
  LinearNodes();
  ~LinearNodes();

  int
  AddTriangularMesh(float* positions, int position_stride, float* normals,
                    int normal_stride, float* uvs, int uv_stride,
                    int num_vertices, int* indices,
                    int num_indices) final override;
  int
  AddSphere(float* position, float radius, int materialID) final override;

  bool
  Build() final override;

  const char*
  GetIntersectionCode() final override;
  const Textures
  GetBVHTexture()
  {
    return m_webgl_textures;
  }
};

struct wr_wide_bvh_node
{
  float px;
  float py;
  float pz;
  char  ex, ey, ez,
    imask; // imask stores which of the children are internal nodes
  unsigned int child_node_base_index; // first referenced child node
  unsigned int triangle_base_index;   // first referenced triangle
  unsigned int
    meta[2]; // store indexing information needed to find the corresponding node
             // or triangle range in the corresponding array. Child nodes:
             // 001xxxxx = xxxxx is the child node slot index plus 24. Leaf
             // nodes: xxxyyyyy = xxx number of triangles (unary format), yyyyy
             // index of the first triangle + triangle_base_index. Empty slot:
             // 000000
  unsigned int childBBOX[12];
};

class WideBVH : public ADS
{

public:
  struct Textures
  {
    unsigned int scene_vertices; // 2D array (posXYZ, UV.x) | (normXYZ, UV.y)
    unsigned int scene_indices;  // (v0,v1,v2, ID)
    unsigned int
      bvh_nodes; // 2D (bbox_min.xyz, prim_child_offset), (bbox_max.xyz, (nPrim
                 // (16bit), axis(8 bit), padd(8 bit)))
    unsigned int attributes[8]; // Attributes buffer
  };

  struct TriangleMesh
  {
    int ID; // ShapeID
    int vertex_offset;
    int num_vertices;
    int triangle_offset;
    int num_triangles;
  };

  std::vector<TriangleMesh> m_triangle_meshes;

  const int         maxPrimsInNode;
  int               m_total_nodes;
  wr_wide_bvh_node* m_linear_nodes;

  int m_shape_id_generator;
  int m_node_texture_size;

  bool m_need_update;

  Textures m_webgl_textures;

  char* intersection_code;

  WideBVH();
  ~WideBVH();

  int
  AddTriangularMesh(float* positions, int position_stride, float* normals,
                    int normal_stride, float* uvs, int uv_stride,
                    int num_vertices, int* indices,
                    int num_indices) final override;
  int
  AddSphere(float* position, float radius, int materialID) final override;

  bool
  Build() final override;

  const char*
  GetIntersectionCode() final override;
  const Textures
  GetBVHTexture()
  {
    return m_webgl_textures;
  }
};

#endif /* _WRAYS_ACCELERATION_DATA_STRUCTURE_H_ */