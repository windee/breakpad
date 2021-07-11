#import <unistd.h>

#import <Foundation/Foundation.h>

#import "sender/HTTPMultipartUpload.h"

#include "sender.h"


namespace dump_helper {

typedef struct {
  NSString *minidumpPath;
  NSString *uploadURLStr;
  NSString *product;
  NSString *version;
  BOOL success;
} Options2;



NSString* stringToNSString(const string& str) {
    return [NSString stringWithCString:str.c_str() encoding:[NSString defaultCStringEncoding]];
}

bool SendCrashReport(const string& server_url, string& file, map<string, string> &params) {
 
  NSString *uploadURLStr = stringToNSString(server_url);
  NSString *minidumpPath = stringToNSString(file);

  NSURL *url = [NSURL URLWithString:uploadURLStr];
  HTTPMultipartUpload *ul = [[HTTPMultipartUpload alloc] initWithURL:url];
  NSMutableDictionary *parameters = [NSMutableDictionary dictionary];

  // Add parameters
    for(auto ite = params.begin(); ite != params.end(); ++ite) {
        NSString* key = stringToNSString(ite->first);
        NSString* value = stringToNSString(ite->second);
        [parameters setObject:value forKey:key];
    }

  [ul setParameters:parameters];

  // Add file
  [ul addFileAtPath:minidumpPath name:@"upload_file_minidump"];

  // Send it
  NSError *error = nil;
  NSData *data = [ul send:&error];
  NSString *result = [[NSString alloc] initWithData:data
                                           encoding:NSUTF8StringEncoding];

  [result release];
  [ul release];

  return (long)[[ul response] statusCode] == 200;
}

}
