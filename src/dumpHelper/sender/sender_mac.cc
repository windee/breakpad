/* This is an example application source code using the multi interface
 * to do a multipart formpost without "blocking". */
#include "sender/sender.h"
#include "common/path_helper.h"
#include <curl/curl.h>

namespace dump_helper {


static size_t cb(char *d, size_t n, size_t l, void *p)
{
  (void)d;
  (void)p;
  return n*l;
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  (void)handle; /* prevent compiler warning */
  (void)userp;
 

  return 0;
}

bool SendCrashReport(const string& url, string& file, map<string, string> &parameters)
{
  CURL *curl;
  
  CURLM *multi_handle;
  int still_running;
  int responseCode = 0;
  
  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;
  struct curl_slist *headerlist=NULL;
//  static const char buf[] = "Expect:";
 
  for(auto ite = parameters.begin(); ite != parameters.end(); ++ite) {
    curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, ite->first.c_str(),
                CURLFORM_COPYCONTENTS, ite->second.c_str(),
                CURLFORM_END);
  }
    
  /* Fill in the filename field */
  curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "filename",
                CURLFORM_COPYCONTENTS,  PathHelper::BaseName(file).c_str(),
                CURLFORM_END);
    
  curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "upload_file_minidump",
                CURLFORM_FILE, file.c_str(),
                CURLFORM_END);
  
  curl = curl_easy_init();
  multi_handle = curl_multi_init();
  
  /* initalize custom header list (stating that Expect: 100-continue is not
     wanted */
  if(curl && multi_handle) {
  
    /* what URL that receives this POST */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
  
    curl_multi_add_handle(multi_handle, curl);
  
    curl_multi_perform(multi_handle, &still_running);
    
  
    do {
      struct timeval timeout;
      int rc; /* select() return code */
  
      fd_set fdread;
      fd_set fdwrite;
      fd_set fdexcep;
      int maxfd = -1;
  
      long curl_timeo = -1;
  
      FD_ZERO(&fdread);
      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);
  
      /* set a suitable timeout to play around with */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
  
      curl_multi_timeout(multi_handle, &curl_timeo);
      if(curl_timeo >= 0) {
        timeout.tv_sec = curl_timeo / 1000;
        if(timeout.tv_sec > 1)
          timeout.tv_sec = 1;
        else
          timeout.tv_usec = (curl_timeo % 1000) * 1000;
      }
  
      /* get file descriptors from the transfers */
      curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
      rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
  
      switch(rc) {
      case -1:
        /* select error */
        break;
      case 0:
      default:
        curl_multi_perform(multi_handle, &still_running);

        break;
      }
    } while(still_running);
      
    
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
  
    curl_multi_cleanup(multi_handle);
  
    /* always cleanup */
    curl_easy_cleanup(curl);
  
    /* then cleanup the formpost chain */
    curl_formfree(formpost);
  
    /* free slist */
    curl_slist_free_all (headerlist);
  }
  return responseCode == 200;
}
}
