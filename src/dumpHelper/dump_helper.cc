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

// minidump_stackwalk.cc: Process a minidump with MinidumpProcessor, printing
// the results, including stack traces.
//
// Author: Mark Mentovai

#include <stdio.h>
#include <string.h>

#include <limits>
#include <string>
#include <vector>

#include "common/path_helper.h"
#include "common/scoped_ptr.h"
#include "common/using_std_string.h"
#include "common/processor/basic_source_line_resolver.h"
#include "common/processor/minidump.h"
#include "common/processor/minidump_processor.h"
#include "common/processor/process_state.h"
#include "common/logging.h"
#include "common/md5.h"
#include "simple_symbol_supplier.h"
#include "stackwalk_common.h"


#ifdef _WIN32
#include "common/getopt.h"
#include "sender/crash_sender.h"
#else

#endif // _WIN32




namespace {

using dump_helper::BasicSourceLineResolver;
using dump_helper::Minidump;
using dump_helper::MinidumpMemoryList;
using dump_helper::MinidumpThreadList;
using dump_helper::MinidumpProcessor;
using dump_helper::ProcessState;
using dump_helper::SimpleSymbolSupplier;
using dump_helper::scoped_ptr;
using dump_helper::Minidump_Info;
using dump_helper::PathHelper;

struct Options {
	string crash_directory;
	std::vector<string> dump_files;
	std::map<string, string> parameters;
};


const std::wstring server_url = L"https://kim-api1.kwaitalk.com/clientlog/log/crash";

bool GenerateDumpInfo(const string& minidump_file, Minidump_Info* dmpInfo) {
  scoped_ptr<SimpleSymbolSupplier> symbol_supplier;
  BasicSourceLineResolver resolver;
  MinidumpProcessor minidump_processor(symbol_supplier.get(), &resolver);

  // Increase the maximum number of threads and regions.
  MinidumpThreadList::set_max_threads(std::numeric_limits<uint32_t>::max());
  MinidumpMemoryList::set_max_regions(std::numeric_limits<uint32_t>::max());
  // Process the minidump.
  Minidump dump(minidump_file);
  if (!dump.Read()) {
     //BPLOG(ERROR) << "Minidump " << dump.path() << " could not be read";
     return false;
  }
  ProcessState process_state;
  if (minidump_processor.Process(&dump, &process_state) !=
      dump_helper::PROCESS_OK) {
    BPLOG(ERROR) << "MinidumpProcessor::Process failed";
    return false;
  }


  dmpInfo->dump_path = minidump_file;
  GetUploadInfo(process_state, dmpInfo);

  //PrintProcessState(process_state, &resolver);

  return true;
}

}  // namespace



static void SetupOptions(int argc, const char *argv[], Options* options) {

  if ((argc - optind) == 0) {
    exit(1);
  }

  options->crash_directory = argv[optind];
  options->dump_files = PathHelper::DumpFiles(options->crash_directory);

  for (int argi = optind + 1; argi < argc; ++argi) {
	  string param = argv[argi];
	  int nPos = param.find_first_of('=');
	  options->parameters[param.substr(0, nPos)] = param.substr(nPos + 1);
  }
}

int main(int argc, const char* argv[]) {
	Options options;
	SetupOptions(argc, argv, &options);

	std::vector<Minidump_Info> vecInfo;

	for (int i = 0; i < options.dump_files.size(); ++i) {
		Minidump_Info info;
		if (GenerateDumpInfo(options.crash_directory + "\\" + options.dump_files[i], &info)) {
			vecInfo.push_back(info);
		}
	}
	std::map<string, string> params = options.parameters;
	params["crash_reason"] = vecInfo[0].crash_reason;
	params["crash_address"] = vecInfo[0].crash_address;
	params["modName"] = vecInfo[0].module_name;
	params["modVersion"] = vecInfo[0].module_version;
	params["modOffset"] = vecInfo[0].module_offset;
	params["stackMd5"] = vecInfo[0].stack_md5;

	std::map<string, string> files;
	files["upload_file_minidump"] = vecInfo[0].dump_path;

	std::wstring report_code;

#ifdef _WIN32
	dump_helper::SendCrashReport(server_url, params, files, &report_code);
#else

#endif // _WIN32

  return 1;
}
