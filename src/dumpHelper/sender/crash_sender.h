// Copyright (c) 2006, Google Inc.
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

#ifndef CLIENT_WINDOWS_SENDER_CRASH_REPORT_SENDER_H__
#define CLIENT_WINDOWS_SENDER_CRASH_REPORT_SENDER_H__

// CrashReportSender is a "static" class which provides an API to upload
// crash reports via HTTP(S).  A crash report is formatted as a multipart POST
// request, which contains a set of caller-supplied string key/value pairs,
// and a minidump file to upload.
//
// To use this library in your project, you will need to link against
// wininet.lib.

#pragma warning( push )
// Disable exception handler warnings.
#pragma warning( disable : 4530 ) 

#include <map>
#include <string>

namespace dump_helper {

using std::string;
using std::wstring;
using std::map;

typedef enum {
  RESULT_FAILED = 0,  // Failed to communicate with the server; try later.
  RESULT_REJECTED,    // Successfully sent the crash report, but the
                      // server rejected it; don't resend this report.
  RESULT_SUCCEEDED,   // The server accepted the crash report.
  RESULT_THROTTLED    // No attempt was made to send the crash report, because
                      // we exceeded the maximum reports per day.
} ReportResult;



ReportResult SendCrashReport(const wstring &url,
                               const map<string, string> &parameters,
                               const map<string, string> &files,
                               wstring *report_code);


}  // namespace dump_helper

#pragma warning( pop )

#endif  // CLIENT_WINDOWS_SENDER_CRASH_REPORT_SENDER_H__
