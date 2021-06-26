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

#include "snapshot/mac/process_snapshot_mac.h"

#include <utility>

#include "util/misc/tri_state.h"

namespace crashpad {

ProcessSnapshotMac::ProcessSnapshotMac()
    : ProcessSnapshot(),
      system_(),
      threads_(),
      modules_(),
      exception_(),
      process_reader_(),
      report_id_(),
      client_id_(),
      snapshot_time_() {
}

ProcessSnapshotMac::~ProcessSnapshotMac() {
}

bool ProcessSnapshotMac::Initialize(task_t task) {

  if (gettimeofday(&snapshot_time_, nullptr) != 0) {
    return false;
  }

  if (!process_reader_.Initialize(task)) {
    return false;
  }

  system_.Initialize(&process_reader_, &snapshot_time_);

  InitializeThreads();
  InitializeModules();

  return true;
}

bool ProcessSnapshotMac::InitializeException(
    exception_behavior_t behavior,
    thread_t exception_thread,
    exception_type_t exception,
    const mach_exception_data_type_t* code,
    mach_msg_type_number_t code_count,
    thread_state_flavor_t flavor,
    ConstThreadState state,
    mach_msg_type_number_t state_count) {

  exception_.reset(new internal::ExceptionSnapshotMac());
  if (!exception_->Initialize(&process_reader_,
                              behavior,
                              exception_thread,
                              exception,
                              code,
                              code_count,
                              flavor,
                              state,
                              state_count)) {
    exception_.reset();
    return false;
  }

  return true;
}

pid_t ProcessSnapshotMac::ProcessID() const {
  return process_reader_.ProcessID();
}

pid_t ProcessSnapshotMac::ParentProcessID() const {
  return process_reader_.ParentProcessID();
}

void ProcessSnapshotMac::SnapshotTime(timeval* snapshot_time) const {
  *snapshot_time = snapshot_time_;
}

void ProcessSnapshotMac::ProcessStartTime(timeval* start_time) const {
  process_reader_.StartTime(start_time);
}

void ProcessSnapshotMac::ProcessCPUTimes(timeval* user_time,
                                         timeval* system_time) const {
  process_reader_.CPUTimes(user_time, system_time);
}

void ProcessSnapshotMac::ReportID(UUID* report_id) const {
  *report_id = report_id_;
}

void ProcessSnapshotMac::ClientID(UUID* client_id) const {
  *client_id = client_id_;
}

const SystemSnapshot* ProcessSnapshotMac::System() const {
  return &system_;
}

std::vector<const ThreadSnapshot*> ProcessSnapshotMac::Threads() const {
  std::vector<const ThreadSnapshot*> threads;
  for (const auto& thread : threads_) {
    threads.push_back(thread.get());
  }
  return threads;
}

std::vector<const ModuleSnapshot*> ProcessSnapshotMac::Modules() const {
  std::vector<const ModuleSnapshot*> modules;
  for (const auto& module : modules_) {
    modules.push_back(module.get());
  }
  return modules;
}

std::vector<UnloadedModuleSnapshot> ProcessSnapshotMac::UnloadedModules()
    const {
  return std::vector<UnloadedModuleSnapshot>();
}

const ExceptionSnapshot* ProcessSnapshotMac::Exception() const {
  return exception_.get();
}

std::vector<const MemoryMapRegionSnapshot*> ProcessSnapshotMac::MemoryMap()
    const {
  return std::vector<const MemoryMapRegionSnapshot*>();
}

std::vector<HandleSnapshot> ProcessSnapshotMac::Handles() const {
  return std::vector<HandleSnapshot>();
}

std::vector<const MemorySnapshot*> ProcessSnapshotMac::ExtraMemory() const {
  return std::vector<const MemorySnapshot*>();
}

const ProcessMemory* ProcessSnapshotMac::Memory() const {
  return process_reader_.Memory();
}

void ProcessSnapshotMac::InitializeThreads() {
  const std::vector<ProcessReaderMac::Thread>& process_reader_threads =
      process_reader_.Threads();
  for (const ProcessReaderMac::Thread& process_reader_thread :
       process_reader_threads) {
    auto thread = std::make_unique<internal::ThreadSnapshotMac>();
    if (thread->Initialize(&process_reader_, process_reader_thread)) {
      threads_.push_back(std::move(thread));
    }
  }
}

void ProcessSnapshotMac::InitializeModules() {
  const std::vector<ProcessReaderMac::Module>& process_reader_modules =
      process_reader_.Modules();
  for (const ProcessReaderMac::Module& process_reader_module :
       process_reader_modules) {
    auto module = std::make_unique<internal::ModuleSnapshotMac>();
    if (module->Initialize(&process_reader_, process_reader_module)) {
      modules_.push_back(std::move(module));
    }
  }
}

}  // namespace crashpad
