/* This is an example application source code using the multi interface
 * to do a multipart formpost without "blocking". */
#include "sender/sender_mac.h"
#include "common/path_helper.h"
#include <stdio.h>
#include <sys/time.h>
  
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
 
//  switch (type) {
//  case CURLINFO_TEXT:
//    fprintf(stderr, "== Info: %s", data);
//  default: /* in case a new one is introduced to shock us */
//    return 0;
//
//  case CURLINFO_HEADER_OUT:
//    text = "=> Send header";
//    break;
//  case CURLINFO_DATA_OUT:
//    text = "=> Send data";
//    break;
//  case CURLINFO_SSL_DATA_OUT:
//    text = "=> Send SSL data";
//    break;
//  case CURLINFO_HEADER_IN:
//    text = "<= Recv header";
//    break;
//  case CURLINFO_DATA_IN:
//    text = "<= Recv data";
//    break;
//  case CURLINFO_SSL_DATA_IN:
//    text = "<= Recv SSL data";
//    break;
//  }
 

  return 0;
}

int SendCrashReport(string url, const map<string, string> &parameters, string file)
{
  CURL *curl;
  
  CURLM *multi_handle;
  int still_running;
  
  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;
  struct curl_slist *headerlist=NULL;
  static const char buf[] = "Expect:";
  
  /* Fill in the file upload field. This makes libcurl load data from
     the given file name when curl_easy_perform() is called. */
//  curl_formadd(&formpost,
//               &lastptr,
//               CURLFORM_COPYNAME, "prod",
//               CURLFORM_COPYCONTENTS, "Kim",
//               CURLFORM_END);
//  curl_formadd(&formpost,
//              &lastptr,
//              CURLFORM_COPYNAME, "appVersion",
//              CURLFORM_COPYCONTENTS, "6.6.6",
//              CURLFORM_END);
//  curl_formadd(&formpost,
//               &lastptr,
//               CURLFORM_COPYNAME, "ver",
//               CURLFORM_COPYCONTENTS, "1.1.1",
//               CURLFORM_END);
 
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
    
// curl_formadd(&formpost,
//              &lastptr,
//              CURLFORM_COPYNAME, "upload_file_minidump",
//              CURLFORM_FILE, file.c_str(),
//              CURLFORM_END);
  
//  /* Fill in the submit field too, even if this is rarely needed */
//  curl_formadd(&formpost,
//               &lastptr,
//               CURLFORM_COPYNAME, "submit",
//               CURLFORM_COPYCONTENTS, "send",
//               CURLFORM_END);
  
  curl = curl_easy_init();
  multi_handle = curl_multi_init();
  
  /* initalize custom header list (stating that Expect: 100-continue is not
     wanted */
  headerlist = curl_slist_append(headerlist, buf);
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
      
      int responseCode = 0;
  
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
  
      /* In a real-world program you OF COURSE check the return code of the
         function calls.  On success, the value of maxfd is guaranteed to be
         greater or equal than -1.  We call select(maxfd + 1, ...), specially in
         case of (maxfd == -1), we call select(0, ...), which is basically equal
         to sleep. */
  
      rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
  
      switch(rc) {
      case -1:
        /* select error */
        break;
      case 0:
      default:
        /* timeout or readable/writable sockets */
        printf("perform!\n");
        curl_multi_perform(multi_handle, &still_running);
        printf("running: %d!\n", still_running);
              
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

        break;
      }
    } while(still_running);
  
    curl_multi_cleanup(multi_handle);
  
    /* always cleanup */
    curl_easy_cleanup(curl);
  
    /* then cleanup the formpost chain */
    curl_formfree(formpost);
  
    /* free slist */
    curl_slist_free_all (headerlist);
  }
    
  std::unique_ptr<int64_t> httpCode(new int64_t);
    // Get the last response code.
  auto ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, httpCode.get());
  return ret;
}
}
