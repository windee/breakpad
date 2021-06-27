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

#include "common/processor/dump_context.h"

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

void DumpContext::Print() {
  if (!valid_) {
    return;
  }

  switch (GetContextCPU()) {
    case MD_CONTEXT_X86: {
      const MDRawContextX86* context_x86 = GetContextX86();
      printf("MDRawContextX86\n");
      printf("  context_flags                = 0x%x\n",
             context_x86->context_flags);
      printf("  dr0                          = 0x%x\n", context_x86->dr0);
      printf("  dr1                          = 0x%x\n", context_x86->dr1);
      printf("  dr2                          = 0x%x\n", context_x86->dr2);
      printf("  dr3                          = 0x%x\n", context_x86->dr3);
      printf("  dr6                          = 0x%x\n", context_x86->dr6);
      printf("  dr7                          = 0x%x\n", context_x86->dr7);
      printf("  float_save.control_word      = 0x%x\n",
             context_x86->float_save.control_word);
      printf("  float_save.status_word       = 0x%x\n",
             context_x86->float_save.status_word);
      printf("  float_save.tag_word          = 0x%x\n",
             context_x86->float_save.tag_word);
      printf("  float_save.error_offset      = 0x%x\n",
             context_x86->float_save.error_offset);
      printf("  float_save.error_selector    = 0x%x\n",
             context_x86->float_save.error_selector);
      printf("  float_save.data_offset       = 0x%x\n",
             context_x86->float_save.data_offset);
      printf("  float_save.data_selector     = 0x%x\n",
             context_x86->float_save.data_selector);
      printf("  float_save.register_area[%2d] = 0x",
             MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE);
      for (unsigned int register_index = 0;
           register_index < MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE;
           ++register_index) {
        printf("%02x", context_x86->float_save.register_area[register_index]);
      }
      printf("\n");
      printf("  float_save.cr0_npx_state     = 0x%x\n",
             context_x86->float_save.cr0_npx_state);
      printf("  gs                           = 0x%x\n", context_x86->gs);
      printf("  fs                           = 0x%x\n", context_x86->fs);
      printf("  es                           = 0x%x\n", context_x86->es);
      printf("  ds                           = 0x%x\n", context_x86->ds);
      printf("  edi                          = 0x%x\n", context_x86->edi);
      printf("  esi                          = 0x%x\n", context_x86->esi);
      printf("  ebx                          = 0x%x\n", context_x86->ebx);
      printf("  edx                          = 0x%x\n", context_x86->edx);
      printf("  ecx                          = 0x%x\n", context_x86->ecx);
      printf("  eax                          = 0x%x\n", context_x86->eax);
      printf("  ebp                          = 0x%x\n", context_x86->ebp);
      printf("  eip                          = 0x%x\n", context_x86->eip);
      printf("  cs                           = 0x%x\n", context_x86->cs);
      printf("  eflags                       = 0x%x\n", context_x86->eflags);
      printf("  esp                          = 0x%x\n", context_x86->esp);
      printf("  ss                           = 0x%x\n", context_x86->ss);
      printf("  extended_registers[%3d]      = 0x",
             MD_CONTEXT_X86_EXTENDED_REGISTERS_SIZE);
      for (unsigned int register_index = 0;
           register_index < MD_CONTEXT_X86_EXTENDED_REGISTERS_SIZE;
           ++register_index) {
        printf("%02x", context_x86->extended_registers[register_index]);
      }
      printf("\n\n");

      break;
    }

    case MD_CONTEXT_AMD64: {
      const MDRawContextAMD64* context_amd64 = GetContextAMD64();
      printf("MDRawContextAMD64\n");
      printf("  p1_home       = 0x%" PRIx64 "\n",
             context_amd64->p1_home);
      printf("  p2_home       = 0x%" PRIx64 "\n",
             context_amd64->p2_home);
      printf("  p3_home       = 0x%" PRIx64 "\n",
             context_amd64->p3_home);
      printf("  p4_home       = 0x%" PRIx64 "\n",
             context_amd64->p4_home);
      printf("  p5_home       = 0x%" PRIx64 "\n",
             context_amd64->p5_home);
      printf("  p6_home       = 0x%" PRIx64 "\n",
             context_amd64->p6_home);
      printf("  context_flags = 0x%x\n",
             context_amd64->context_flags);
      printf("  mx_csr        = 0x%x\n",
             context_amd64->mx_csr);
      printf("  cs            = 0x%x\n", context_amd64->cs);
      printf("  ds            = 0x%x\n", context_amd64->ds);
      printf("  es            = 0x%x\n", context_amd64->es);
      printf("  fs            = 0x%x\n", context_amd64->fs);
      printf("  gs            = 0x%x\n", context_amd64->gs);
      printf("  ss            = 0x%x\n", context_amd64->ss);
      printf("  eflags        = 0x%x\n", context_amd64->eflags);
      printf("  dr0           = 0x%" PRIx64 "\n", context_amd64->dr0);
      printf("  dr1           = 0x%" PRIx64 "\n", context_amd64->dr1);
      printf("  dr2           = 0x%" PRIx64 "\n", context_amd64->dr2);
      printf("  dr3           = 0x%" PRIx64 "\n", context_amd64->dr3);
      printf("  dr6           = 0x%" PRIx64 "\n", context_amd64->dr6);
      printf("  dr7           = 0x%" PRIx64 "\n", context_amd64->dr7);
      printf("  rax           = 0x%" PRIx64 "\n", context_amd64->rax);
      printf("  rcx           = 0x%" PRIx64 "\n", context_amd64->rcx);
      printf("  rdx           = 0x%" PRIx64 "\n", context_amd64->rdx);
      printf("  rbx           = 0x%" PRIx64 "\n", context_amd64->rbx);
      printf("  rsp           = 0x%" PRIx64 "\n", context_amd64->rsp);
      printf("  rbp           = 0x%" PRIx64 "\n", context_amd64->rbp);
      printf("  rsi           = 0x%" PRIx64 "\n", context_amd64->rsi);
      printf("  rdi           = 0x%" PRIx64 "\n", context_amd64->rdi);
      printf("  r8            = 0x%" PRIx64 "\n", context_amd64->r8);
      printf("  r9            = 0x%" PRIx64 "\n", context_amd64->r9);
      printf("  r10           = 0x%" PRIx64 "\n", context_amd64->r10);
      printf("  r11           = 0x%" PRIx64 "\n", context_amd64->r11);
      printf("  r12           = 0x%" PRIx64 "\n", context_amd64->r12);
      printf("  r13           = 0x%" PRIx64 "\n", context_amd64->r13);
      printf("  r14           = 0x%" PRIx64 "\n", context_amd64->r14);
      printf("  r15           = 0x%" PRIx64 "\n", context_amd64->r15);
      printf("  rip           = 0x%" PRIx64 "\n", context_amd64->rip);
      // TODO: print xmm, vector, debug registers
      printf("\n");
      break;
    }

    case MD_CONTEXT_ARM: {
      const MDRawContextARM* context_arm = GetContextARM();
      const char * const names[] = {
        "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
        "r8",  "r9",  "r10", "r11", "r12", "sp",  "lr",  "pc",
      };
      printf("MDRawContextARM\n");
      printf("  context_flags        = 0x%x\n",
             context_arm->context_flags);
      for (unsigned int ireg_index = 0;
           ireg_index < MD_CONTEXT_ARM_GPR_COUNT;
           ++ireg_index) {
        printf("  %-3s                  = 0x%x\n",
               names[ireg_index], context_arm->iregs[ireg_index]);
      }
      printf("  cpsr                 = 0x%x\n", context_arm->cpsr);
      printf("  float_save.fpscr     = 0x%" PRIx64 "\n",
             context_arm->float_save.fpscr);
      for (unsigned int fpr_index = 0;
           fpr_index < MD_FLOATINGSAVEAREA_ARM_FPR_COUNT;
           ++fpr_index) {
        printf("  float_save.regs[%2d]  = 0x%" PRIx64 "\n",
               fpr_index, context_arm->float_save.regs[fpr_index]);
      }
      for (unsigned int fpe_index = 0;
           fpe_index < MD_FLOATINGSAVEAREA_ARM_FPEXTRA_COUNT;
           ++fpe_index) {
        printf("  float_save.extra[%2d] = 0x%" PRIx32 "\n",
               fpe_index, context_arm->float_save.extra[fpe_index]);
      }

      break;
    }

    case MD_CONTEXT_ARM64: {
      const MDRawContextARM64* context_arm64 = GetContextARM64();
      printf("MDRawContextARM64\n");
      printf("  context_flags       = 0x%x\n",
             context_arm64->context_flags);
      for (unsigned int ireg_index = 0;
           ireg_index < MD_CONTEXT_ARM64_GPR_COUNT;
           ++ireg_index) {
        printf("  iregs[%2d]            = 0x%" PRIx64 "\n",
               ireg_index, context_arm64->iregs[ireg_index]);
      }
      printf("  cpsr                = 0x%x\n", context_arm64->cpsr);
      printf("  float_save.fpsr     = 0x%x\n", context_arm64->float_save.fpsr);
      printf("  float_save.fpcr     = 0x%x\n", context_arm64->float_save.fpcr);

      for (unsigned int freg_index = 0;
           freg_index < MD_FLOATINGSAVEAREA_ARM64_FPR_COUNT;
           ++freg_index) {
        uint128_struct fp_value = context_arm64->float_save.regs[freg_index];
        printf("  float_save.regs[%2d]            = 0x%" PRIx64 "%" PRIx64 "\n",
               freg_index, fp_value.high, fp_value.low);
      }

      break;
    }

    default: {
      break;
    }
  }
}

}  // namespace dump_helper
