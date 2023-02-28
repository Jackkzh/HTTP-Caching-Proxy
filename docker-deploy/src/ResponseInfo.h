#ifndef RESPONSEINFO_H
#define RESPONSEINFO_H

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "helper.h"

class ResponseInfo {
 public:
  std::string content;
  int status_code;
  std::string re_direct_url;
  std::string response;
  std::string request_time;

  // Cache fields
  int maxAge;  //A boolean indicating whether the response can be cached or not.
  std::string expirationTime;  //The expiration time of the response
  std::string lastModified;
  std::string eTag;

  int smaxAge;
  bool mustRevalidate;
  bool noCache;  //The maximum time in seconds that the response can be cached.
  bool noStore;
  bool isPublic;
  bool isPrivate;
  int maxStale;
  std::string date;

  // Cache policy fields
  int cache_status;
  int freshLifeTime;
  int currAge;
  bool isBadGateway;

  // Chunk and Content-Length fields
  bool is_chunk;
  int content_length;
  std::string content_type;

  ResponseInfo() :
      is_chunk(false),
      content_length(-1),
      smaxAge(-1),
      maxAge(-1),
      noCache(false),
      noStore(false),
      isPublic(true),
      isPrivate(false),
      expirationTime(""),
      lastModified(""),
      eTag(""),
      date(""),
      freshLifeTime(-1),
      currAge(-1),
      isBadGateway(false){};

  ResponseInfo(std::string buffer) :
      is_chunk(false),
      content_length(-1),
      smaxAge(-1),
      maxAge(-1),
      noCache(false),
      noStore(false),
      isPublic(true),
      isPrivate(false),
      expirationTime(""),
      lastModified(""),
      eTag(""),
      date(""),
      freshLifeTime(-1),
      currAge(-1),
      isBadGateway(false){};

  void parseResponse(std::string & buffer, std::string requestTime) {
    request_time = requestTime;
    setContentLength(buffer);
    setChunked(buffer);
    setStatusCode(buffer);
    checkStatus(buffer);
    setContentType(buffer);
    setCacheControl(buffer);
  }

  bool checkBadGateway(int client_fd, int thread_id);
  void printCacheFields();

  bool isFresh(std::string response_time, int maxStale = 0) {
    setFreshLifeTime(maxStale);
    setCurrentAge(response_time);
    if (freshLifeTime + maxStale > currAge) {
      return true;
    }
    else {
      return false;
    }
  }

  bool isCacheable(int thread_id);

  void logCat(int thread_id);

 private:
  void setContentLength(std::string & buffer);
  void setChunked(std::string & buffer);
  void setStatusCode(std::string & buffer);
  void checkStatus(std::string & buffer);
  void setContentType(std::string & buffer);
  void setCacheControl(std::string & buffer);
  void setFreshLifeTime(int maxStale = 0);
  void setCurrentAge(std::string response_time);
};

#endif
