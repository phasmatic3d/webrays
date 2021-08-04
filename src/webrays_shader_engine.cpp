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

#include "webrays/webrays.h"

#include <string>

wr_string_buffer
wr_string_buffer_create(wr_size reserve)
{
  std::string* sb = new std::string;

  sb->reserve(reserve);

  return static_cast<wr_string_buffer>(sb);
}

wr_error
wr_string_buffer_destroy(wr_string_buffer* string_buffer)
{
  std::string* sb = static_cast<std::string*>(*string_buffer);
  delete sb;
  *string_buffer = nullptr;

  return nullptr;
}

wr_error
wr_string_buffer_append_range(wr_string_buffer string_buffer, const char* begin,
                              const char* end)
{
  std::string* sb = static_cast<std::string*>(string_buffer);

  sb->append(begin, end);

  return nullptr;
}

wr_error
wr_string_buffer_append(wr_string_buffer string_buffer, const char* data)
{
  std::string* sb = static_cast<std::string*>(string_buffer);

  (*sb) += data;

  return nullptr;
}

wr_error
wr_string_buffer_prepend(wr_string_buffer string_buffer, const char* data)
{
  std::string* sb = static_cast<std::string*>(string_buffer);

  sb->insert(0, std::string(data));

  return nullptr;
}

wr_error
wr_string_buffer_clear(wr_string_buffer string_buffer)
{
  std::string* sb = static_cast<std::string*>(string_buffer);

  sb->clear();

  return nullptr;
}

wr_error
wr_string_buffer_appendln(wr_string_buffer string_buffer, const char* data)
{
  std::string* sb = static_cast<std::string*>(string_buffer);

  (*sb) += data;
  (*sb) += "\n";

  return nullptr;
}

wr_error
wr_string_buffer_appendsp(wr_string_buffer string_buffer, const char* data)
{
  std::string* sb = static_cast<std::string*>(string_buffer);

  (*sb) += data;
  (*sb) += " ";

  return nullptr;
}

const char*
wr_string_buffer_data(wr_string_buffer string_buffer)
{
  return (static_cast<std::string*>(string_buffer))->data();
}

void
wr_string_buffer_pretty_print(wr_string_buffer string_buffer)
{
  std::string* sb         = static_cast<std::string*>(string_buffer);
  int          line_count = 1;
  printf("%04d", line_count++);
  for (const char c : *sb) {
    if ('\n' == c)
      printf("%c%04d. ", c, line_count++);
    else
      printf("%c", c);
  }
}

wr_error
wr_string_buffer_pop_back(wr_string_buffer string_buffer)
{
  std::string* sb = static_cast<std::string*>(string_buffer);

  sb->pop_back();

  return nullptr;
}

char
wr_string_buffer_back(wr_string_buffer string_buffer)
{
  std::string* sb = static_cast<std::string*>(string_buffer);
  return sb->back();
}
