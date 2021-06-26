// Copyright 2015 The Crashpad Authors. All rights reserved.
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

#include "util/mach/exception_types.h"

namespace crashpad {

exception_type_t ExcCrashRecoverOriginalException(
    mach_exception_code_t code_0,
    mach_exception_code_t* original_code_0,
    int* signal) {
  // 10.9.4 xnu-2422.110.17/bsd/kern/kern_exit.c proc_prepareexit() sets code[0]
  // based on the signal value, original exception type, and low 20 bits of the
  // original code[0] before calling xnu-2422.110.17/osfmk/kern/exception.c
  // task_exception_notify() to raise an EXC_CRASH.
  //
  // The list of core-generating signals (as used in proc_prepareexit()’s call
  // to hassigprop()) is in 10.9.4 xnu-2422.110.17/bsd/sys/signalvar.h sigprop:
  // entires with SA_CORE are in the set. These signals are SIGQUIT, SIGILL,
  // SIGTRAP, SIGABRT, SIGEMT, SIGFPE, SIGBUS, SIGSEGV, and SIGSYS. Processes
  // killed for code-signing reasons will be killed by SIGKILL and are also
  // eligible for EXC_CRASH handling, but processes killed by SIGKILL for other
  // reasons are not.
  if (signal) {
    *signal = (code_0 >> 24) & 0xff;
  }

  if (original_code_0) {
    *original_code_0 = code_0 & 0xfffff;
  }

  return (code_0 >> 20) & 0xf;
}

}  // namespace crashpad
