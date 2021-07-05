// Copyright (c) 2010 Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// dump_context.cc: A (mini/micro)dump context.
//
// See dump_context.h for documentation.

#include "parser/dump_context.h"

#include <assert.h>

#ifdef _WIN32
#include <io.h>
#else  // _WIN32
#include <unistd.h>
#endif  // _WIN32

#include "common/stdio_wrapper.h"

namespace dump_helper {

DumpContext::DumpContext() : context_(),
                             context_flags_(0) { }

DumpContext::~DumpContext() {
  FreeContext();
}

uint32_t DumpContext::GetContextCPU() const {
  if (!valid_) {
    // Don't log a message, GetContextCPU can be legitimately called with
    // valid_ false by FreeContext, which is called by Read.
    return 0;
  }

  return context_flags_ & MD_CONTEXT_CPU_MASK;
}

uint32_t DumpContext::GetContextFlags() const {
  return context_flags_;
}

const MDRawContextX86* DumpContext::GetContextX86() const {
  if (GetContextCPU() != MD_CONTEXT_X86) {
    return NULL;
  }

  return context_.x86;
}

const MDRawContextAMD64* DumpContext::GetContextAMD64() const {
  if (GetContextCPU() != MD_CONTEXT_AMD64) {
    return NULL;
  }

  return context_.amd64;
}

const MDRawContextARM* DumpContext::GetContextARM() const {
  if (GetContextCPU() != MD_CONTEXT_ARM) {
    return NULL;
  }

  return context_.arm;
}

const MDRawContextARM64* DumpContext::GetContextARM64() const {
  if (GetContextCPU() != MD_CONTEXT_ARM64) {
    return NULL;
  }

  return context_.arm64;
}

bool DumpContext::GetInstructionPointer(uint64_t* ip) const {
  assert(ip);
  *ip = 0;

  if (!valid_) {
    return false;
  }

  switch (GetContextCPU()) {
  case MD_CONTEXT_AMD64:
    *ip = GetContextAMD64()->rip;
    break;
  case MD_CONTEXT_ARM:
    *ip = GetContextARM()->iregs[MD_CONTEXT_ARM_REG_PC];
    break;
  case MD_CONTEXT_ARM64:
    *ip = GetContextARM64()->iregs[MD_CONTEXT_ARM64_REG_PC];
    break;
  case MD_CONTEXT_X86:
    *ip = GetContextX86()->eip;
    break;
  default:
    // This should never happen.
    return false;
  }
  return true;
}

bool DumpContext::GetStackPointer(uint64_t* sp) const {
  assert(sp);
  *sp = 0;

  if (!valid_) {
    return false;
  }

  switch (GetContextCPU()) {
  case MD_CONTEXT_AMD64:
    *sp = GetContextAMD64()->rsp;
    break;
  case MD_CONTEXT_ARM:
    *sp = GetContextARM()->iregs[MD_CONTEXT_ARM_REG_SP];
    break;
  case MD_CONTEXT_ARM64:
    *sp = GetContextARM64()->iregs[MD_CONTEXT_ARM64_REG_SP];
    break;
  case MD_CONTEXT_X86:
    *sp = GetContextX86()->esp;
    break;
  default:
    // This should never happen.
    return false;
  }
  return true;
}

void DumpContext::SetContextFlags(uint32_t context_flags) {
  context_flags_ = context_flags;
}

void DumpContext::SetContextX86(MDRawContextX86* x86) {
  context_.x86 = x86;
}

void DumpContext::SetContextAMD64(MDRawContextAMD64* amd64) {
  context_.amd64 = amd64;
}

void DumpContext::SetContextARM(MDRawContextARM* arm) {
  context_.arm = arm;
}

void DumpContext::SetContextARM64(MDRawContextARM64* arm64) {
  context_.arm64 = arm64;
}

void DumpContext::FreeContext() {
  switch (GetContextCPU()) {
    case MD_CONTEXT_X86:
      delete context_.x86;
      break;

    case MD_CONTEXT_AMD64:
      delete context_.amd64;
      break;

    case MD_CONTEXT_ARM:
      delete context_.arm;
      break;

    case MD_CONTEXT_ARM64:
      delete context_.arm64;
      break;

    default:
      // There is no context record (valid_ is false) or there's a
      // context record for an unknown CPU (shouldn't happen, only known
      // records are stored by Read).
      break;
  }

  context_flags_ = 0;
  context_.base = NULL;
}

}  // namespace dump_helper
