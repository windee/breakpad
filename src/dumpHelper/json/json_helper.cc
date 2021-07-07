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

#include "json_helper.h"
#include <algorithm>
#include <iostream>
#include <fstream>

namespace dump_helper {


string JsonHelper::json_dir;
string JsonHelper::root_name;


void JsonHelper::init(const char* dir, const char* name) {
    json_dir = dir;
    root_name = name;
}
 
void JsonHelper::readRoot(Json::Value &root){
  std::ifstream ifs(json_dir + "/" + root_name);
    if(ifs && ifs.is_open()) {
        Json::CharReaderBuilder reader;
        string err;
        bool ret;
        ret = Json::parseFromStream(reader, ifs, &root, &err);
        ifs.close();
    }
}

void JsonHelper::writeRoot(Json::Value &root)
{
  std::ofstream fout(json_dir + "/" + root_name);
    if (fout) {
        fout << root.toStyledString();
        fout.close();
    }
}

void JsonHelper::addFile(string& file){
    Json::Value root;
    readRoot(root);
    Json::Value& array = root[root_name];
    array.append(file.c_str());
    writeRoot(root);
}

vector<string> JsonHelper::getFiles() {
	vector<string> files;

    Json::Value root;
    readRoot(root);
    Json::Value& array = root[root_name];

	if (array.size() == 0) {
		return files;
	}
    
    for(unsigned int i = 0; i < array.size() ; i++){
        string file = array[i].asString();
        if (remove((json_dir + "/" + file).c_str()) != 0) {
            files.push_back(file);
        }
    }

    array.clear();
    for(unsigned int i = 0; i < files.size() ; i++){
        array.append(files[i].c_str());
    }

    writeRoot(root);
    
    return files;
}

string JsonHelper::stringfy(vector<Minidump_Info>& infos) {
    Json::Value root;
    Json::Value array;
        
    for(unsigned int i = 0; i < infos.size() ; i++){
        Json::Value item;
        item["module_name"] = infos[i].module_name;
        item["module_offset"] = infos[i].module_offset;
        item["crash_reason"] = infos[i].crash_reason;
        array.append(item);
    }

    root["completed_files"] = array;
    return root.toStyledString();
}
}  // namespace dump_helper
