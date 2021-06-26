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

#include "minidump/minidump_string_writer.h"

#include <utility>

#include "minidump/minidump_writer_util.h"
#include "util/file/file_writer.h"
#include "util/numeric/safe_assignment.h"

namespace crashpad {
namespace internal {

template <typename Traits>
MinidumpStringWriter<Traits>::MinidumpStringWriter()
    : MinidumpWritable(), string_base_(new MinidumpStringType()), string_() {
}

template <typename Traits>
MinidumpStringWriter<Traits>::~MinidumpStringWriter() {
}

template <typename Traits>
bool MinidumpStringWriter<Traits>::Freeze() {

  if (!MinidumpWritable::Freeze()) {
    return false;
  }

  size_t string_bytes = string_.size() * sizeof(string_[0]);
  if (!AssignIfInRange(&string_base_->Length, string_bytes)) {
    return false;
  }

  return true;
}

template <typename Traits>
size_t MinidumpStringWriter<Traits>::SizeOfObject() {

  // Include the NUL terminator.
  return sizeof(*string_base_) + (string_.size() + 1) * sizeof(string_[0]);
}

template <typename Traits>
bool MinidumpStringWriter<Traits>::WriteObject(
    FileWriterInterface* file_writer) {

  // The string’s length is stored in string_base_, and its data is stored in
  // string_. Write them both.
  WritableIoVec iov;
  iov.iov_base = string_base_.get();
  iov.iov_len = sizeof(*string_base_);
  std::vector<WritableIoVec> iovecs(1, iov);

  // Include the NUL terminator.
  iov.iov_base = &string_[0];
  iov.iov_len = (string_.size() + 1) * sizeof(string_[0]);
  iovecs.push_back(iov);

  return file_writer->WriteIoVec(&iovecs);
}

// Explicit template instantiation of the forms of MinidumpStringWriter<> used
// as base classes.
template class MinidumpStringWriter<MinidumpStringWriterUTF16Traits>;
template class MinidumpStringWriter<MinidumpStringWriterUTF8Traits>;

MinidumpUTF16StringWriter::~MinidumpUTF16StringWriter() {
}

void MinidumpUTF16StringWriter::SetUTF8(const std::string& string_utf8) {

  set_string(MinidumpWriterUtil::ConvertUTF8ToUTF16(string_utf8));
}

MinidumpUTF8StringWriter::~MinidumpUTF8StringWriter() {
}

template <typename MinidumpStringWriterType>
MinidumpStringListWriter<MinidumpStringWriterType>::MinidumpStringListWriter()
    : MinidumpRVAListWriter() {
}

template <typename MinidumpStringWriterType>
MinidumpStringListWriter<
    MinidumpStringWriterType>::~MinidumpStringListWriter() {
}

template <typename MinidumpStringWriterType>
void MinidumpStringListWriter<MinidumpStringWriterType>::InitializeFromVector(
    const std::vector<std::string>& vector) {

  for (const std::string& string : vector) {
    AddStringUTF8(string);
  }
}

template <typename MinidumpStringWriterType>
void MinidumpStringListWriter<MinidumpStringWriterType>::AddStringUTF8(
    const std::string& string_utf8) {
  auto string_writer = std::make_unique<MinidumpStringWriterType>();
  string_writer->SetUTF8(string_utf8);
  AddChild(std::move(string_writer));
}

template <typename MinidumpStringWriterType>
bool MinidumpStringListWriter<MinidumpStringWriterType>::IsUseful() const {
  return !IsEmpty();
}

// Explicit template instantiation of the forms of MinidumpStringListWriter<>
// used as type aliases.
template class MinidumpStringListWriter<MinidumpUTF16StringWriter>;
template class MinidumpStringListWriter<MinidumpUTF8StringWriter>;

}  // namespace internal
}  // namespace crashpad
