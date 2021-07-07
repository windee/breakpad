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

#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "common/path_helper.h"
#include "common/scoped_ptr.h"
#include "common/md5.h"
#include "parser/minidump.h"
#include "parser/minidump_processor.h"
#include "parser/process_state.h"
#include "parser/stackwalk_common.h"
#include "json/json_helper.h"
#include "sender/sender.h"

#ifdef _WIN32
#include "common/getopt.h"
#endif // _WIN32


using std::string;
using std::map;
using namespace dump_helper;


struct Options {
    string server_url;
	string crash_directory;
	vector<string> dump_files;
	map<string, string> parameters;
};


bool getMacProcessType(ProcessState& state, Minidump_Info* pInfo) {
    auto module = state.modules()->GetMainModule();
    string filename = PathHelper::FileName(module->code_file());
    
    size_t start = filename.find('(');
    size_t end = filename.find(')');
    
    if (filename.find("electron") == -1 && filename.find("kim") == -1) {
        return false;
    }
    
    if (start == -1 || end == -1){
        pInfo->process_type = "browser";
    } else {
        pInfo->process_type = filename.substr(start + 1, end - start - 1);
    }
    
    return true;
}

bool getWinProcessType(ProcessState& state, Minidump_Info* pInfo) {
    const CodeModules*  modules = state.modules();
    for (unsigned int i = 0; i < modules->module_count(); i ++) {
        auto module = modules->GetModuleAtIndex(i);
        if (module == NULL)
        {
            continue;
        }
        
        string filename = PathHelper::FileName(module->code_file());
        
        if (filename == "kimcastcontroller.dll") {
            pInfo->process_type = "browser";
            return true;
        }
    }
    
    pInfo->process_type = "renderer";
    return true;
}

bool isValidState(ProcessState& state) {
    auto module = state.modules()->GetMainModule();
    string filename = PathHelper::FileName(module->code_file());
    
    if (filename == "kim.exe") {
        string version = module->version();
        size_t pos = version.find_last_of('.');
        if (pos != -1) {
            string mainVersion = version.substr(0, pos);
            int v1 = 0, v2 = 0, v3 = 0;
            sscanf(mainVersion.c_str(), "%d.%d.%d", &v1, &v2, &v3);
            //拿不到版本，直接放过
            if (v1 == 0 && v2 == 0 && v3 == 0) {
                return true;
            }
            //拿到版本，最低版本应该是1.0.91
            if (v1 < 1 || (v1 == 1 && v2 == 0 && v3 < 91)) {
                return false;
            }

            return true;
        }
    }
      
    if (filename.find("electron") == -1 && filename.find("kim") == -1) {
        return false;
    }
    
    return true;
}

bool ParseMinidump(const string& minidump_file, Minidump_Info* dmpInfo) {
  MinidumpProcessor minidump_processor;
  // Increase the maximum number of threads and regions.
  MinidumpThreadList::set_max_threads(std::numeric_limits<uint32_t>::max());
  MinidumpMemoryList::set_max_regions(std::numeric_limits<uint32_t>::max());
  // Process the minidump.
  Minidump dump(minidump_file);
  if (!dump.Read()) {
     return false;
  }
  ProcessState process_state;
  if (minidump_processor.Process(&dump, &process_state) !=
      dump_helper::PROCESS_OK) {
    return false;
  }

  if (!isValidState(process_state)) {
    return false;
  }
    
#ifdef _WIN32
    getWinProcessType(process_state, dmpInfo);
#else
    getMacProcessType(process_state, dmpInfo);
#endif // _WIN32

  dmpInfo->dump_path = minidump_file;
  GetUploadInfo(process_state, dmpInfo);

  return true;
}

const uint32_t max_days = 7;
const size_t max_counts = 5;

static void SetupOptions(int argc, const char *argv[], Options* options) {

  if ((argc - optind) < 1) {
    exit(1);
  }

  options->server_url = argv[optind];
  options->crash_directory = argv[optind + 1];
  dump_helper::JsonHelper::init(options->crash_directory.c_str(), "complete_file");
  options->dump_files = PathHelper::DumpFiles(options->crash_directory, max_days);

  for (int argi = optind + 2; argi < argc; ++argi) {
	  string param = argv[argi];
	  size_t nPos = param.find_first_of('=');
	  options->parameters[param.substr(0, nPos)] = param.substr(nPos + 1);
  }
}

int main(int argc, const char* argv[]) {
    Options options;
	SetupOptions(argc, argv, &options);

	vector<Minidump_Info> vecInfo;
	int count = 0;

	for (int i = 0; i < options.dump_files.size() && count < max_counts; ++i) {
		Minidump_Info info;
        if (!ParseMinidump(options.crash_directory + "/" + options.dump_files[i], &info)) {
            continue;
        }

		map<string, string> params = options.parameters;
		params["crashReason"] = info.crash_reason;
		params["crashAddress"] = info.crash_address;
		params["moduleName"] = info.module_name;
		params["moduleVersion"] = info.module_version;
		params["moduleOffset"] = info.module_offset;
		params["stackMd5"] = info.stack_md5;
        
        auto ite = params.find("process_type");
        if (ite == params.end() || ite->second != info.process_type) {
			params["process_type"] = info.process_type;
			params["processTitle"] = info.process_type == "renderer" ? "IM" : "Main";
        }

        if (!SendCrashReport(options.server_url, info.dump_path, params)) {
            continue;
        } else {
            vecInfo.push_back(info);
        }

        count++;
        if (remove(info.dump_path.c_str())) {
            JsonHelper::addFile(options.dump_files[i]);
        }
	}

	printf("%s", JsonHelper::stringfy(vecInfo).c_str());

	return 0;
}

