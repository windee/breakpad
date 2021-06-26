// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/scoped_file.h"

#include <stdio.h>
#include <unistd.h>

namespace base {
namespace internal {

#if defined(OS_POSIX)
void ScopedFDCloseTraits::Free(int fd) {
    close(fd);
}
#endif  // OS_POSIX

void ScopedFILECloser::operator()(FILE* file) const {
  if (file) {
      fclose(file);
  }
}

}  // namespace internal
}  // namespace base
