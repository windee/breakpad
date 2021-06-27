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

// stackwalk_common.cc: Module shared by the {micro,mini}dump_stackwalck
// executables to print the content of dumps (w/ stack traces) on the console.
//
// Author: Mark Mentovai

#include "stackwalk_common.h"

#include <assert.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <set>
#include <sstream>

#include "common/stdio_wrapper.h"
#include "common/processor/call_stack.h"
#include "common/processor/code_module.h"
#include "common/processor/code_modules.h"
#include "common/processor/process_state.h"
#include "common/processor/stack_frame_cpu.h"
#include "common/path_helper.h"
#include "common/md5.h"


namespace dump_helper {

namespace {

using std::vector;
using std::string;
// PrintStack prints the call stack in |stack| to stdout, in a reasonably
// useful form.  Module, function, and source file names are displayed if
// they are available.  The code offset to the base code address of the
// source line, function, or module is printed, preferring them in that
// order.  If no source line, function, or module information is available,
// an absolute code offset is printed.
//
// If |cpu| is a recognized CPU name, relevant register state for each stack
// frame printed is also output, if available.
static void PrintStack(const CallStack *stack,
                       const string &cpu,
                       const MemoryRegion* memory,
                       const CodeModules* modules) {
  int frame_count = stack->frames()->size();
  if (frame_count == 0) {
    printf(" <no frames>\n");
  }
  for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
    const StackFrame *frame = stack->frames()->at(frame_index);
    printf("%2d  ", frame_index);

    uint64_t instruction_address = frame->ReturnAddress();

    if (frame->module) {
      printf("%s", PathHelper::FileName(frame->module->code_file()).c_str());
      if (!frame->function_name.empty()) {
        printf("!%s", frame->function_name.c_str());
        if (!frame->source_file_name.empty()) {
          string source_file = PathHelper::FileName(frame->source_file_name);
          printf(" [%s : %d + 0x%" PRIx64 "]",
                 source_file.c_str(),
                 frame->source_line,
                 instruction_address - frame->source_line_base);
        } else {
          printf(" + 0x%" PRIx64, instruction_address - frame->function_base);
        }
      } else {
        printf(" + 0x%" PRIx64,
               instruction_address - frame->module->base_address());
      }
    } else {
      printf("0x%" PRIx64, instruction_address);
    }
    printf("\n ");
  }
}

// ContainsModule checks whether a given |module| is in the vector
// |modules_without_symbols|.
static bool ContainsModule(
    const vector<const CodeModule*> *modules,
    const CodeModule *module) {
  assert(modules);
  assert(module);
  vector<const CodeModule*>::const_iterator iter;
  for (iter = modules->begin(); iter != modules->end(); ++iter) {
    if (module->debug_file().compare((*iter)->debug_file()) == 0 &&
        module->debug_identifier().compare((*iter)->debug_identifier()) == 0) {
      return true;
    }
  }
  return false;
}

// PrintModule prints a single |module| to stdout.
// |modules_without_symbols| should contain the list of modules that were
// confirmed to be missing their symbols during the stack walk.
static void PrintModule(
    const CodeModule *module,
    const vector<const CodeModule*> *modules_without_symbols,
    const vector<const CodeModule*> *modules_with_corrupt_symbols,
    uint64_t main_address) {
  string symbol_issues;
  if (ContainsModule(modules_without_symbols, module)) {
    symbol_issues = "  (WARNING: No symbols, " +
        PathHelper::FileName(module->debug_file()) + ", " +
        module->debug_identifier() + ")";
  } else if (ContainsModule(modules_with_corrupt_symbols, module)) {
    symbol_issues = "  (WARNING: Corrupt symbols, " +
        PathHelper::FileName(module->debug_file()) + ", " +
        module->debug_identifier() + ")";
  }
  uint64_t base_address = module->base_address();
  printf("0x%08" PRIx64 " - 0x%08" PRIx64 "  %s  %s%s%s\n",
         base_address, base_address + module->size() - 1,
         PathHelper::FileName(module->code_file()).c_str(),
         module->version().empty() ? "???" : module->version().c_str(),
         main_address != 0 && base_address == main_address ? "  (main)" : "",
         symbol_issues.c_str());
}

// PrintModules prints the list of all loaded |modules| to stdout.
// |modules_without_symbols| should contain the list of modules that were
// confirmed to be missing their symbols during the stack walk.
static void PrintModules(
    const CodeModules *modules,
    const vector<const CodeModule*> *modules_without_symbols,
    const vector<const CodeModule*> *modules_with_corrupt_symbols) {
  if (!modules)
    return;

  printf("\n");
  printf("Loaded modules:\n");

  uint64_t main_address = 0;
  const CodeModule *main_module = modules->GetMainModule();
  if (main_module) {
    main_address = main_module->base_address();
  }

  unsigned int module_count = modules->module_count();
  for (unsigned int module_sequence = 0;
       module_sequence < module_count;
       ++module_sequence) {
    const CodeModule *module = modules->GetModuleAtSequence(module_sequence);
    PrintModule(module, modules_without_symbols, modules_with_corrupt_symbols,
                main_address);
  }
}

}  // namespace

void PrintProcessState(const ProcessState& process_state) {
  // Print OS and CPU information.
  string cpu = process_state.system_info()->cpu;
  string cpu_info = process_state.system_info()->cpu_info;
  printf("Operating system: %s\n", process_state.system_info()->os.c_str());
  printf("                  %s\n",
         process_state.system_info()->os_version.c_str());
  printf("CPU: %s\n", cpu.c_str());
  if (!cpu_info.empty()) {
    // This field is optional.
    printf("     %s\n", cpu_info.c_str());
  }
  printf("     %d CPU%s\n",
         process_state.system_info()->cpu_count,
         process_state.system_info()->cpu_count != 1 ? "s" : "");
  printf("\n");

  // Print GPU information
  string gl_version = process_state.system_info()->gl_version;
  string gl_vendor = process_state.system_info()->gl_vendor;
  string gl_renderer = process_state.system_info()->gl_renderer;
  printf("GPU:");
  if (!gl_version.empty() || !gl_vendor.empty() || !gl_renderer.empty()) {
    printf(" %s\n", gl_version.c_str());
    printf("     %s\n", gl_vendor.c_str());
    printf("     %s\n", gl_renderer.c_str());
  } else {
    printf(" UNKNOWN\n");
  }
  printf("\n");

  // Print crash information.
  if (process_state.crashed()) {
    printf("Crash reason:  %s\n", process_state.crash_reason().c_str());
    printf("Crash address: 0x%" PRIx64 "\n", process_state.crash_address());
  } else {
    printf("No crash\n");
  }

  string assertion = process_state.assertion();
  if (!assertion.empty()) {
    printf("Assertion: %s\n", assertion.c_str());
  }

  // Compute process uptime if the process creation and crash times are
  // available in the dump.
  if (process_state.time_date_stamp() != 0 &&
      process_state.process_create_time() != 0 &&
      process_state.time_date_stamp() >= process_state.process_create_time()) {
    printf("Process uptime: %d seconds\n",
           process_state.time_date_stamp() -
               process_state.process_create_time());
  } else {
    printf("Process uptime: not available\n");
  }

  // If the thread that requested the dump is known, print it first.
  int requesting_thread = process_state.requesting_thread();
  if (requesting_thread != -1) {
    printf("\n");
    printf("Thread %d (%s)\n",
          requesting_thread,
          process_state.crashed() ? "crashed" :
                                    "requested dump, did not crash");
    PrintStack(process_state.threads()->at(requesting_thread), cpu,
               process_state.thread_memory_regions()->at(requesting_thread),
               process_state.modules());
  }


  PrintModules(process_state.modules(),
               process_state.modules_without_symbols(),
               process_state.modules_with_corrupt_symbols());
}

bool GetModuleInfo(const StackFrame* pFrame, Minidump_Info* dmpInfo, int index) {

	uint64_t instruction_address = pFrame->ReturnAddress();

	if (index == 0) {
		char str[80];
		sprintf(str, "0x%" PRIx64, instruction_address);
		dmpInfo->crash_address = str;
	}

	if (pFrame->module ) {
		char module_offset[80];
		sprintf(module_offset, "0x%" PRIx64, instruction_address - pFrame->module->base_address());

		dmpInfo->module_name = PathHelper::FileName(pFrame->module->code_file());
		dmpInfo->module_version = pFrame->module->version();

		dmpInfo->module_offset = module_offset;
		return true;
	}

	return false;
}


std::set<string> g_SystemModules = { "kernelbase.dll", "ntdll.dll", "kernel32.dll" };

void GetCallStack(const CallStack* stack, int max_count, Minidump_Info* dmpInfo) {
	std::string strStack;

	int frame_count = stack->frames()->size();
	bool has_module_info = false;

	for (int frame_index = 0; frame_index < frame_count && frame_index < max_count; ++frame_index) {
		const StackFrame* frame = stack->frames()->at(frame_index);
		uint64_t instruction_address = frame->ReturnAddress();
		if (!has_module_info) {
			has_module_info = GetModuleInfo(frame, dmpInfo, frame_index);
		}

		if (frame->module) {
			string filename = PathHelper::FileName(frame->module->code_file());
			strStack += filename;
			if (g_SystemModules.find(filename) == g_SystemModules.end()) {
				char module_offset[80];
				sprintf(module_offset, "0x%" PRIx64, instruction_address - frame->module->base_address());
				strStack += module_offset;
			}
		}
		else {
			char str[80];
			sprintf(str, "0x%" PRIx64, instruction_address);
			strStack += str;
		}
	}

	md5::MD5 md;
	dmpInfo->stack_md5 = md.digestString((char*)strStack.c_str());
}

void GetUploadInfo(const ProcessState& process_state, Minidump_Info* dmpInfo) {

	// Print crash information.
	if (process_state.crashed()) {
		dmpInfo->crash_reason = process_state.crash_reason();

		int requesting_thread = process_state.requesting_thread();
		if (requesting_thread != -1) {
			CallStack* pStack = process_state.threads()->at(requesting_thread);
			GetCallStack(pStack, 10, dmpInfo);
		}
	}
	else {
		dmpInfo->crash_reason = "ANR";
		CallStack* pStack = process_state.threads()->at(0);
		GetCallStack(pStack, 10, dmpInfo);
	}
}

}  // namespace dump_helper
