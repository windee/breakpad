// Copyright 2014 The Crashpad Authors. All rights reserved.
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

#include "util/mach/scoped_task_suspend.h"


namespace crashpad {

ScopedTaskSuspend::ScopedTaskSuspend(task_t task) : task_(task) {

  kern_return_t kr = task_suspend(task_);
  if (kr != KERN_SUCCESS) {
    task_ = TASK_NULL;
  }
}

ScopedTaskSuspend::~ScopedTaskSuspend() {
  if (task_ != TASK_NULL) {
    task_resume(task_);
  }
}

}  // namespace crashpad
