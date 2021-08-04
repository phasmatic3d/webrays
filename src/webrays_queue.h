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

#ifndef _WRAYS_QUEUE_H_
#define _WRAYS_QUEUE_H_

#include <webrays/webrays.h>

#define WR_FIELD_OFFSET(type, field) ((unsigned long long)&(((type*)0)->field))

typedef struct wr_queue
{
  wr_size size;
  wr_size capacity;
  wr_byte data[1];
} wr_queue;

void
wrays_queue_expand(void** address, wr_size capacity);

#define wrays_queue_init(address, capacity)                                    \
  {                                                                            \
    address = WR_NULL;                                                         \
    wrays_queue_expand((void**)&address, sizeof(*address) * capacity);         \
  }

#define wrays_queue_size(address)                                              \
  ((wr_queue*)((wr_byte*)address - WR_FIELD_OFFSET(wr_queue, data)))->size

#define wrays_queue_capacity(address)                                          \
  (((wr_queue*)((wr_byte*)address - WR_FIELD_OFFSET(wr_queue, data)))          \
     ->capacity /                                                              \
   sizeof(*address))

#define wrays_queue_last(address)                                              \
  ((wrays_queue_size(address)) ? address[wrays_queue_size(address) - 1]        \
                               : GRB_NULL)

#define wrays_queue_remove(address)                                            \
  if (wrays_queue_size(address))                                               \
  wrays_queue_size(address)--

#define wrays_queue_push(address, value)                                       \
  {                                                                            \
    if (wrays_queue_size(address) == wrays_queue_capacity(address)) {          \
                                                                               \
      wrays_queue_expand((void**)&address, sizeof(*address) *                  \
                                             wrays_queue_capacity(address) *   \
                                             2);                               \
    }                                                                          \
    address[wrays_queue_size(address)++] = value;                              \
  }

#endif /* _WRAYS_QUEUE_H_ */
