/* This is an example application source code using the multi interface
 * to do a multipart formpost without "blocking". */
#ifndef DUMPHLPER_UPLOAD_H_
#define DUMPHLPER_UPLOAD_H_

#include <string>
#include <map>


namespace dump_helper {
using std::string;
using std::map;

int SendCrashReport(string url, const map<string, string> &parameters, string file);
}
#endif  // DUMPHLPER_UPLOAD_H_
