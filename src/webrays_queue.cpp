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

#include "webrays_queue.h"

#include <cstdlib>

void
wrays_queue_expand(void** address, wr_size capacity)
{
  if (0 == capacity)
    return;

  wr_queue* queue;
  void*     address_deref = *address;
  if (address_deref == WR_NULL) {
    queue       = (wr_queue*)malloc(WR_FIELD_OFFSET(wr_queue, data) + capacity);
    queue->size = 0;
  } else {
    queue = (wr_queue*)realloc(
      ((unsigned char*)address_deref - WR_FIELD_OFFSET(wr_queue, data)),
      WR_FIELD_OFFSET(wr_queue, data) + capacity);
  }

  queue->capacity = capacity;
  queue = (wr_queue*)((unsigned char*)queue + WR_FIELD_OFFSET(wr_queue, data));

  *address = queue;
}