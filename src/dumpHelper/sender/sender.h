/* This is an example application source code using the multi interface
 * to do a multipart formpost without "blocking". */
#ifndef DUMP_HELPER_UPLOAD_H_
#define DUMP_HELPER_UPLOAD_H_

#include <string>
#include <map>


namespace dump_helper {
using std::string;
using std::map;

bool SendCrashReport(const string& url, string& file, map<string, string> &parameters);
}
#endif  // DUMP_HELPER_UPLOAD_H_
