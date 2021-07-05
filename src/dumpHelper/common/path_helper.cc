// Copyright 2017, Google Inc.
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

#include "path_helper.h"
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "json/json_helper.h"



#ifdef _WIN32
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif // _WIN32

namespace dump_helper {


string PathHelper::BaseName(const string& path) {
  char* path_tmp = strdup(path.c_str());
  assert(path_tmp);

#ifdef _WIN32
  string result(PathFindFileNameA(path_tmp));
#else
  string result(basename(path_tmp));
#endif // _WIN32

  free(path_tmp);
  return result;
}

string PathHelper::DirName(const string& path) {
  char* path_tmp = strdup(path.c_str());
  assert(path_tmp);

#ifdef _WIN32
  PathRemoveFileSpecA(path_tmp);
  string result(path_tmp);
#else
  string result(dirname(path_tmp));
#endif // _WIN32

  free(path_tmp);

  return result;
}

string PathHelper::ToLower(const string& name) {
	string result = name;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

string PathHelper::FileName(const string& path) {
	string::size_type slash = path.rfind('/');
	string::size_type backslash = path.rfind('\\');

	string::size_type file_start = 0;
	if (slash != string::npos &&
		(backslash == string::npos || slash > backslash)) {
		file_start = slash + 1;
	}
	else if (backslash != string::npos) {
		file_start = backslash + 1;
	}

	return ToLower(path.substr(file_start));
}


vector<string> PathHelper::DumpFiles(const string& dir) {
	vector<string> vec;
    vector<string> completed = JsonHelper::getFiles();
#ifdef _WIN32
	WIN32_FIND_DATAA findData;
	memset(&findData, 0, sizeof(WIN32_FIND_DATAA));
	HANDLE hFind = FindFirstFileA((dir + "/*.*").c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return vec;
	}

	while (FindNextFileA(hFind, &findData) == TRUE)
	{
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			const string filename = findData.cFileName;
			string suffix = filename.substr(filename.find_last_of('.') + 1);
            auto ite = find(completed.begin(), completed.end(), filename);
			if ((suffix == "dmp" || suffix == "DMP" ) && ite == completed.end()) {
				vec.push_back(filename);
			}
		}
	}
#else
	DIR* dp;
	struct dirent* entry;
	struct stat statbuf;

	if ((dp = opendir(dir.c_str())) == NULL)
	{
		return vec;
	}

	while ((entry = readdir(dp)) != NULL)
	{
		std::string subdir = dir + "/";
		subdir += entry->d_name;
		lstat(subdir.c_str(), &statbuf);

		if (!S_ISDIR(statbuf.st_mode))
		{
			const string filename = entry->d_name;
			string suffix = filename.substr(filename.find_last_of('.') + 1);
			auto ite = find(completed.begin(), completed.end(), filename);
			if ((suffix == "dmp" || suffix == "DMP" ) && ite == completed.end()) {
				vec.push_back(filename);
			}
		}
	}

	closedir(dp);
#endif // _WIN32
	return vec;
}

}  // namespace dump_helper
