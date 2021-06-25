// Copyright 2014 The Crashpad Authors. All rights reserved.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "minidump/minidump_file_writer.h"
#include "util/file/file_writer.h"
#include "util/process/process_id.h"
#include "util/stdlib/string_number_conversion.h"


#include <unistd.h>

#include "util/posix/drop_privileges.h"


#include <mach/mach.h>
#include "base/mac/scoped_mach_port.h"
#include "snapshot/mac/process_snapshot_mac.h"
#include "util/mach/scoped_task_suspend.h"
#include "util/mach/task_for_pid.h"


namespace crashpad {
namespace {



int GenerateDumpMain(int pid, std::string path, bool pend = true) {

  task_t task = TaskForPID(pid);
  if (task == TASK_NULL) {
    return EXIT_FAILURE;
  }
  base::mac::ScopedMachSendRight task_owner(task);

  // This tool may have been installed as a setuid binary so that TaskForPID()
  // could succeed. Drop any privileges now that they’re no longer necessary.
  DropPrivileges();

  if (pid == getpid()) {
    if (pend) {
      LOG(ERROR) << "cannot suspend myself";
      return EXIT_FAILURE;
    }
    LOG(WARNING) << "operating on myself";
  }


  if (path.empty()) {
    path = base::StringPrintf("minidump.%" PRI_PROCESS_ID,
                                           pid);
  }

  {
    std::unique_ptr<ScopedTaskSuspend> suspend;
    if (pend) {
      suspend.reset(new ScopedTaskSuspend(task));
    }


    ProcessSnapshotMac process_snapshot;
    if (!process_snapshot.Initialize(task)) {
      return EXIT_FAILURE;
    }

    FileWriter file_writer;
    base::FilePath dump_path(path);
    if (!file_writer.Open(dump_path,
                          FileWriteMode::kTruncateOrCreate,
                          FilePermissions::kWorldReadable)) {
      return EXIT_FAILURE;
    }

    MinidumpFileWriter minidump;
    minidump.InitializeFromSnapshot(&process_snapshot);
    if (!minidump.WriteEverything(&file_writer)) {
      file_writer.Close();
      if (unlink(path.c_str()) != 0) {
        PLOG(ERROR) << "unlink";
      }
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

}  // namespace
}  // namespace crashpad


int main(int argc, char* argv[]) {
  return crashpad::GenerateDumpMain(23695, "/Users/lichuanle/desktop/123.dmp");
}

