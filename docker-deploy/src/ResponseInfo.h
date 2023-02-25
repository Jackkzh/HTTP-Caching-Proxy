#ifndef RESPONSEINFO_H
#define RESPONSEINFO_H

#include <boost/algorithm/string.hpp>
#include <boost/beast.hpp>
#include <boost/date_time.hpp>
#include <boost/regex.hpp>

#include <string>
#include <vector>

#include "helper.h"

class ResponseInfo {
 public:
  std::string content;
  int status_code;
  std::string re_direct_url;

  // Cache fields
  int maxAge;  //A boolean indicating whether the response can be cached or not.
  std::string expirationTime;  //The expiration time of the response
  std::string lastModified;
  std::string eTag;

  bool mustRevalidate;
  bool noCache;  //The maximum time in seconds that the response can be cached.
  bool noStore;
  bool isPublic;
  bool isPrivate;
  int maxStale;

  // Cache policy fields
  int cache_status;
  int freshLifeTime;

  // Chunk and Content-Length fields
  bool is_chunk;
  int content_length;

  ResponseInfo() :
      is_chunk(false),
      content_length(-1),
      maxAge(-1),
      noCache(false),
      expirationTime(""),
      lastModified(""),
      eTag(""),
      freshLifeTime(-1){};

  ResponseInfo(std::string buffer) :
      is_chunk(false),
      content_length(-1),
      maxAge(-1),
      noCache(false),
      expirationTime(""),
      lastModified(""),
      eTag(""),
      freshLifeTime(-1){};

  void parseResponse(std::string & buffer) {
    setContentLength(buffer);
    setChunked(buffer);
    setStatusCode(buffer);
    checkStatus(buffer);
    setCacheControl(buffer);
  }

  void setContentLength(std::string & buffer);
  void setContent(std::string msgbody) { content = msgbody; }
  void setChunked(std::string & buffer);
  void setStatusCode(std::string & buffer);
  void checkStatus(std::string & buffer);
  void setCacheControl(std::string & buffer);
  void printCacheFields();
  void setFreshLifeTime(std::string & buffer, int maxStale = 0);

  bool isCacheable();
};

#endif