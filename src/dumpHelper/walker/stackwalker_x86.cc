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

// stackwalker_x86.cc: x86-specific stackwalker.
//
// See stackwalker_x86.h for documentation.
//
// Author: Mark Mentovai

#include <assert.h>
#include <string>

#include "common/scoped_ptr.h"
#include "parser/call_stack.h"
#include "parser/code_modules.h"
#include "common/memory_region.h"
#include "parser/stack_frame_cpu.h"
#include "common/postfix_evaluator-inl.h"
#include "walker/stackwalker_x86.h"
#include "common/cfi_frame_info.h"

namespace dump_helper {

// Max reasonable size for a single x86 frame is 128 KB.  This value is used in
// a heuristic for recovering of the EBP chain after a scan for return address.
// This value is based on a stack frame size histogram built for a set of
// popular third party libraries which suggests that 99.5% of all frames are
// smaller than 128 KB.
static const uint32_t kMaxReasonableGapBetweenFrames = 128 * 1024;

const StackwalkerX86::CFIWalker::RegisterSet
StackwalkerX86::cfi_register_map_[] = {
  // It may seem like $eip and $esp are callee-saves, because (with Unix or
  // cdecl calling conventions) the callee is responsible for having them
  // restored upon return. But the callee_saves flags here really means
  // that the walker should assume they're unchanged if the CFI doesn't
  // mention them, which is clearly wrong for $eip and $esp.
  { "$eip", ".ra",  false,
    StackFrameX86::CONTEXT_VALID_EIP, &MDRawContextX86::eip },
  { "$esp", ".cfa", false,
    StackFrameX86::CONTEXT_VALID_ESP, &MDRawContextX86::esp },
  { "$ebp", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EBP, &MDRawContextX86::ebp },
  { "$eax", NULL,   false,
    StackFrameX86::CONTEXT_VALID_EAX, &MDRawContextX86::eax },
  { "$ebx", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EBX, &MDRawContextX86::ebx },
  { "$ecx", NULL,   false,
    StackFrameX86::CONTEXT_VALID_ECX, &MDRawContextX86::ecx },
  { "$edx", NULL,   false,
    StackFrameX86::CONTEXT_VALID_EDX, &MDRawContextX86::edx },
  { "$esi", NULL,   true,
    StackFrameX86::CONTEXT_VALID_ESI, &MDRawContextX86::esi },
  { "$edi", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EDI, &MDRawContextX86::edi },
};

StackwalkerX86::StackwalkerX86(const SystemInfo* system_info,
                               const MDRawContextX86* context,
                               MemoryRegion* memory,
                               const CodeModules* modules)
    : Stackwalker(system_info, memory, modules),
      context_(context),
      cfi_walker_(cfi_register_map_,
                  (sizeof(cfi_register_map_) / sizeof(cfi_register_map_[0]))) {
  if (memory_ && memory_->GetBase() + memory_->GetSize() - 1 > 0xffffffff) {
    // The x86 is a 32-bit CPU, the limits of the supplied stack are invalid.
    // Mark memory_ = NULL, which will cause stackwalking to fail.
    memory_ = NULL;
  }
}

StackFrameX86::~StackFrameX86() {
  if (cfi_frame_info)
    delete cfi_frame_info;
  cfi_frame_info = NULL;
}

uint64_t StackFrameX86::ReturnAddress() const {
  assert(context_validity & StackFrameX86::CONTEXT_VALID_EIP);
  return context.eip;
}

StackFrame* StackwalkerX86::GetContextFrame() {
  if (!context_) {
    return NULL;
  }

  StackFrameX86* frame = new StackFrameX86();

  // The instruction pointer is stored directly in a register, so pull it
  // straight out of the CPU context structure.
  frame->context = *context_;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_ALL;
  frame->trust = StackFrame::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.eip;

  return frame;
}

StackFrameX86* StackwalkerX86::GetCallerByCFIFrameInfo(
    const vector<StackFrame*> &frames,
    CFIFrameInfo* cfi_frame_info) {
  StackFrameX86* last_frame = static_cast<StackFrameX86*>(frames.back());
  last_frame->cfi_frame_info = cfi_frame_info;

  scoped_ptr<StackFrameX86> frame(new StackFrameX86());
  if (!cfi_walker_
      .FindCallerRegisters(*memory_, *cfi_frame_info,
                           last_frame->context, last_frame->context_validity,
                           &frame->context, &frame->context_validity))
    return NULL;

  // Make sure we recovered all the essentials.
  static const int essentials = (StackFrameX86::CONTEXT_VALID_EIP
                                 | StackFrameX86::CONTEXT_VALID_ESP
                                 | StackFrameX86::CONTEXT_VALID_EBP);
  if ((frame->context_validity & essentials) != essentials)
    return NULL;

  frame->trust = StackFrame::FRAME_TRUST_CFI;

  return frame.release();
}

StackFrameX86* StackwalkerX86::GetCallerByEBPAtBase(
    const vector<StackFrame*> &frames,
    bool stack_scan_allowed) {
  StackFrame::FrameTrust trust;
  StackFrameX86* last_frame = static_cast<StackFrameX86*>(frames.back());
  uint32_t last_esp = last_frame->context.esp;
  uint32_t last_ebp = last_frame->context.ebp;

  // Assume that the standard %ebp-using x86 calling convention is in
  // use.
  //
  // The typical x86 calling convention, when frame pointers are present,
  // is for the calling procedure to use CALL, which pushes the return
  // address onto the stack and sets the instruction pointer (%eip) to
  // the entry point of the called routine.  The called routine then
  // PUSHes the calling routine's frame pointer (%ebp) onto the stack
  // before copying the stack pointer (%esp) to the frame pointer (%ebp).
  // Therefore, the calling procedure's frame pointer is always available
  // by dereferencing the called procedure's frame pointer, and the return
  // address is always available at the memory location immediately above
  // the address pointed to by the called procedure's frame pointer.  The
  // calling procedure's stack pointer (%esp) is 8 higher than the value
  // of the called procedure's frame pointer at the time the calling
  // procedure made the CALL: 4 bytes for the return address pushed by the
  // CALL itself, and 4 bytes for the callee's PUSH of the caller's frame
  // pointer.
  //
  // %eip_new = *(%ebp_old + 4)
  // %esp_new = %ebp_old + 8
  // %ebp_new = *(%ebp_old)

  uint32_t caller_eip, caller_esp, caller_ebp;

  if (memory_->GetMemoryAtAddress(last_ebp + 4, &caller_eip) &&
      memory_->GetMemoryAtAddress(last_ebp, &caller_ebp)) {
    caller_esp = last_ebp + 8;
    trust = StackFrame::FRAME_TRUST_FP;
  } else {
    // We couldn't read the memory %ebp refers to. It may be that %ebp
    // is pointing to non-stack memory. We'll scan the stack for a
    // return address. This can happen if last_frame is executing code
    // for a module for which we don't have symbols, and that module
    // is compiled without a frame pointer.
    if (!stack_scan_allowed
        || !ScanForReturnAddress(last_esp, &caller_esp, &caller_eip,
                                 frames.size() == 1 /* is_context_frame */)) {
      // if we can't find an instruction pointer even with stack scanning,
      // give up.
      return NULL;
    }

    // ScanForReturnAddress found a reasonable return address. Advance %esp to
    // the location immediately above the one where the return address was
    // found.
    caller_esp += 4;
    // Try to restore the %ebp chain.  The caller %ebp should be stored at a
    // location immediately below the one where the return address was found.
    // A valid caller %ebp must be greater than the address where it is stored
    // and the gap between the two adjacent frames should be reasonable.
    uint32_t restored_ebp_chain = caller_esp - 8;
    if (!memory_->GetMemoryAtAddress(restored_ebp_chain, &caller_ebp) ||
        caller_ebp <= restored_ebp_chain ||
        caller_ebp - restored_ebp_chain > kMaxReasonableGapBetweenFrames) {
      // The restored %ebp chain doesn't appear to be valid.
      // Assume that %ebp is unchanged.
      caller_ebp = last_ebp;
    }

    trust = StackFrame::FRAME_TRUST_SCAN;
  }

  // Create a new stack frame (ownership will be transferred to the caller)
  // and fill it in.
  StackFrameX86* frame = new StackFrameX86();

  frame->trust = trust;
  frame->context = last_frame->context;
  frame->context.eip = caller_eip;
  frame->context.esp = caller_esp;
  frame->context.ebp = caller_ebp;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_EIP |
                            StackFrameX86::CONTEXT_VALID_ESP |
                            StackFrameX86::CONTEXT_VALID_EBP;

  return frame;
}

StackFrame* StackwalkerX86::GetCallerFrame(const CallStack* stack,
                                           bool stack_scan_allowed) {
  if (!memory_ || !stack) {
    return NULL;
  }

  const vector<StackFrame*> &frames = *stack->frames();
  StackFrameX86* last_frame = static_cast<StackFrameX86*>(frames.back());
  scoped_ptr<StackFrameX86> new_frame;

  // Otherwise, hope that the program was using a traditional frame structure.
  if (!new_frame.get())
    new_frame.reset(GetCallerByEBPAtBase(frames, stack_scan_allowed));

  // If nothing worked, tell the caller.
  if (!new_frame.get())
    return NULL;

  // Should we terminate the stack walk? (end-of-stack or broken invariant)
  if (TerminateWalk(new_frame->context.eip,
                    new_frame->context.esp,
                    last_frame->context.esp,
                    frames.size() == 1)) {
    return NULL;
  }

  // new_frame->context.eip is the return address, which is the instruction
  // after the CALL that caused us to arrive at the callee. Set
  // new_frame->instruction to one less than that, so it points within the
  // CALL instruction. See StackFrame::instruction for details, and
  // StackFrameAMD64::ReturnAddress.
  new_frame->instruction = new_frame->context.eip - 1;

  return new_frame.release();
}

}  // namespace dump_helper
