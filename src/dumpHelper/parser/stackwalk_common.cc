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
#include "parser/call_stack.h"
#include "parser/code_module.h"
#include "parser/code_modules.h"
#include "parser/process_state.h"
#include "parser/stack_frame_cpu.h"
#include "common/path_helper.h"
#include "common/md5.h"


namespace dump_helper {

#ifdef _WIN32
std::set<string> g_SystemModules = { "kernelbase.dll", "ntdll.dll", "kernel32.dll" };
#else
std::set<string> g_SystemModules = { "libsystem_platform.dylib", "libsystem_kernel.dylib", "libsystem_c.dylib", "CoreFoundation", "Foundation", "libsystem_pthread.dylib",
    "libc++.1.dylib", "libc++abi.dylib"
};
#endif

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

		return g_SystemModules.find(dmpInfo->module_name) == g_SystemModules.end();
	}

	return false;
}

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
			sprintf(str, "0x%" PRIx32, 0xffffffff);
			strStack += str;
		}
	}

	md5::MD5 md;
	dmpInfo->stack_md5 = md.digestString((char*)strStack.c_str());
}

void GetUploadInfo(const ProcessState& process_state, Minidump_Info* dmpInfo) {
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
