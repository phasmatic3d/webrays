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

#ifndef _WRAYS_TOP_LEVEL_ACCELERATION_DATA_STRUCTURE_H_
#define _WRAYS_TOP_LEVEL_ACCELERATION_DATA_STRUCTURE_H_

#include <vector>

struct Instance
{
  float transform[12];
  int   blas_offset;
  int   pad0;
  int   pad1;
  int   pad2;
};

class TLAS
{
public:
  std::vector<Instance> m_instances;

protected:
};

#endif /* _WRAYS_TOP_LEVEL_ACCELERATION_DATA_STRUCTURE_H_ */
