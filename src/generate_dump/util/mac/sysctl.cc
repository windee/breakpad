// Copyright 2020 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/mac/sysctl.h"

#include <errno.h>
#include <sys/sysctl.h>
#include <sys/types.h>


namespace crashpad {

std::string ReadStringSysctlByName(const char* name, bool may_log_enoent) {
  size_t buf_len;
  if (sysctlbyname(name, nullptr, &buf_len, nullptr, 0) != 0) {
    return std::string();
  }


  std::string value(buf_len - 1, '\0');
  if (sysctlbyname(name, &value[0], &buf_len, nullptr, 0) != 0) {
    return std::string();
  }

  return value;
}

}  // namespace crashpad
