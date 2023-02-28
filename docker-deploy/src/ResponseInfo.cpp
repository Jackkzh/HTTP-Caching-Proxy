#include "ResponseInfo.h"

/**
 * Set the content length of the HTTP response
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::setContentLength(std::string & buffer) {
  response = buffer;
  std::cout << response << std::endl;
  content_length = -1;
  std::size_t end = buffer.find("\r\n\r\n");
  if (end != std::string::npos) {
    std::string headers = buffer.substr(0, end);

    // use Boost to find if the header contains "Content-Length: "
    boost::regex re("Content-Length:\\s*(\\d+)");
    boost::smatch what;
    if (boost::regex_search(headers, what, re)) {
      // get the string after "Content-Length: "
      content_length = std::stoi(what[1].str());
    }
    else {
      isBadGateway = true;
    }
  }
}

/**
 * Determine whether the HTTP response is chunked based on "Transfer-Encoding".
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::setChunked(std::string & buffer) {
  std::size_t pos = buffer.find("\r\n\r\n");
  if (pos != std::string::npos) {
    std::string headers = buffer.substr(0, pos);

    // use Boost to find if the header contains "Transfer-Encoding: chunked"
    boost::regex re("Transfer-Encoding: chunked");
    boost::smatch what;
    if (boost::regex_search(headers, what, re)) {
      is_chunk = true;
    }
    else {
      is_chunk = false;
    }
  }
}

/**
 * Set the status code of the HTTP response based on the "HTTP/1.x"
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::setStatusCode(std::string & buffer) {
  std::size_t status_start = buffer.find("HTTP/1.");
  if (status_start != std::string::npos) {
    status_code = stoi(buffer.substr(status_start + 9, 3));
  }
}

void ResponseInfo::setContentType(std::string & buffer) {
  size_t content_start = buffer.find("Content-Type:");
  if (content_start != std::string::npos) {
    size_t content_end = buffer.find(";", content_start);
    content_type = buffer.substr(content_start + 14, content_end - content_start - 14);
  }
  else {
    isBadGateway = true;
  }
}

/**
 * Check the status code of the HTTP response and take action
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::checkStatus(std::string & buffer) {
  if (status_code == 301) {
    // move permanently
    std::size_t location_start = buffer.find("Location:");
    if (location_start != std::string::npos) {
      std::size_t location_end = buffer.find("\r\n", location_start);
      std::string new_location =
          buffer.substr(location_start + 10, location_end - location_start - 10);

      // send new GET request to new location
      re_direct_url = new_location;
    }
  }
}

/**
 * Set cache control fields based on the "Cache-Control",
 * "Expires", "Last-Modified", and "ETag" headers in the HTTP response.
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::setCacheControl(std::string & buffer) {
  TimeMake t;
  std::cout << buffer << std::endl;
  std::size_t end = buffer.find("\r\n\r\n");
  if (end != std::string::npos) {
    std::string headers = buffer.substr(0, end);

    boost::regex d("Date:\\s*([^\r\n]*)");
    boost::smatch whatd;
    if (boost::regex_search(headers, whatd, d)) {
      // get the string after "Cache-Control: "
      date = whatd[1].str();
    }
    else {
      isBadGateway = true;
    }

    // use Boost to find if the header contains "Cache-Control: "
    boost::regex re("Cache-Control:\\s*([^\\r\\n]+)");
    boost::smatch what;
    if (boost::regex_search(headers, what, re)) {
      // get the string after "Cache-Control: "
      std::string cache_control = what[1].str();

      // check if cache-control contains "must-revalidate" directive
      if (cache_control.find("must-revalidate") != std::string::npos) {
        mustRevalidate = true;
      }

      // check if cache-control contains "no-cache" directive
      if (cache_control.find("no-cache") != std::string::npos) {
        noCache = true;
      }

      // check if cache-control contains "no-store" directive
      if (cache_control.find("no-store") != std::string::npos) {
        noStore = true;
      }

      // check if cache-control contains "public" directive
      if (cache_control.find("public") != std::string::npos) {
        isPublic = true;
      }
      else if (cache_control.find("private") != std::string::npos) {
        isPrivate = true;
      }

      // use Boost to find if the header contains "Expires: "
      re = boost::regex("Expires:\\s*([^\r\n]*)");
      if (boost::regex_search(headers, what, re)) {
        if (what[1].str() !=
            "-1") {  // -1 in the Expires header means that the response should not be cached
          expirationTime = t.convertGMT(what[1].str());
        }
        // std::cout << "expirationTime: " << expirationTime << std::endl;
        // std::cout << "Convert expirationTime: " << t.convertGMT(expirationTime)
        //           << std::endl;
      }

      // use Boost to find if the header contains "max-age=seconds"
      re = boost::regex("max-age=(\\d+)");
      if (boost::regex_search(cache_control, what, re)) {
        maxAge = std::stoi(what[1].str());
        TimeMake t;
        expirationTime = t.getTime(maxAge);
      }

      // use Boost to find if the header contains "max-age=seconds"
      re = boost::regex("s-maxage=(\\d+)");
      if (boost::regex_search(cache_control, what, re)) {
        smaxAge = std::stoi(what[1].str());
      }

      // use Boost to find if the header contains "Last-Modified: "
      re = boost::regex("Last-Modified:\\s*([^\r\n]*)");
      if (boost::regex_search(headers, what, re)) {
        lastModified = t.convertGMT(what[1].str());
        // std::cout << "lastModified: " << lastModified << std::endl;
      }

      // use Boost to find if the header contains "ETag: "
      re = boost::regex("ETag:\\s*\"([^\"]*)\"");
      if (boost::regex_search(headers, what, re)) {
        std::string etagTmp = what[1].str();
        etagTmp.erase(std::remove(etagTmp.begin(), etagTmp.end(), '\"'), etagTmp.end());
        eTag = etagTmp;
        // std::cout << "eTag: " << eTag << std::endl;
      }
    }
    else {  //if there is no Cache-Control, we set it uncacheable
      isPublic = false;
    }
  }
}

/**
 * Print the cache fields of the HTTP response 
 * maxAge, noCache, expirationTime, lastModified, eTag.
 */
void ResponseInfo::printCacheFields() {
  std::cout << "date: " << date << std::endl;
  std::cout << "public: " << isPublic << std::endl;
  std::cout << "noCache: " << noCache << std::endl;
  std::cout << "noStore: " << noStore << std::endl;
  std::cout << "mustRevalidate: " << mustRevalidate << std::endl;
  std::cout << "smaxAge: " << smaxAge << std::endl;
  std::cout << "maxAge: " << maxAge << std::endl;
  std::cout << "noCache: " << noCache << std::endl;
  std::cout << "expirationTime: " << expirationTime << std::endl;
  std::cout << "lastModified: " << lastModified << std::endl;
  std::cout << "eTag: " << eTag << std::endl;
}

/**
 * @param buffer the buffer containing the HTTP response
 * @param maxStale If max-stale is assigned a value, then the client is willing 
 * to accept a response that has exceeded its freshness lifetime
 * by no more than the specified number of seconds.
*/
void ResponseInfo::setFreshLifeTime(int maxStale) {
  TimeMake t;
  if (smaxAge != -1) {
    freshLifeTime = smaxAge + maxStale;
  }
  else if (maxAge != -1) {
    freshLifeTime = maxAge + maxStale;
    std::cout << freshLifeTime << std::endl;
  }
  else if (expirationTime != "") {
    freshLifeTime = t.timeMinus(expirationTime, date) + maxStale;
  }
  else {
    freshLifeTime = maxStale;
  }
}

/**
 * @param buffer the buffer containing the HTTP response
 * @param request_time The current value of the clock at 
 * the host at the time the request resulting in the stored 
 * response was made.
 * @param response_time The current value of the clock at 
 * the host at the time the response was received.
*/
void ResponseInfo::setCurrentAge(std::string response_time) {
  std::string buffer = response;
  TimeMake t;
  int age = 0;
  std::string date = "";
  std::size_t end = buffer.find("\r\n\r\n");
  if (end != std::string::npos) {
    std::string headers = buffer.substr(0, end);

    boost::regex rea("Age:(\\d+)");
    boost::smatch whata;
    if (boost::regex_search(headers, whata, rea)) {
      age = std::stoi(whata[1].str());
    }

    boost::regex re("Date:\\s*([^\r\n]*)");
    boost::smatch what;
    if (boost::regex_search(headers, what, re)) {
      std::string date = t.convertGMT(what[1].str());
    }
  }
  int apparent_age = std::max(0, t.timeMinus(response_time, date));
  int response_delay = t.timeMinus(response_time, request_time);
  int corrected_age_value = age + response_delay;
  int corrected_initial_age = std::max(apparent_age, corrected_age_value);
  int resident_time = t.timeMinus(t.getTime(), response_time);
  currAge = corrected_initial_age + resident_time;
}

bool ResponseInfo::isCacheable(int thread_id) {
  Logger log;
  TimeMake t;
  if (noStore) {
    std::string msg = std::to_string(thread_id) + ": not cacheable because no-store.";
    log.log(msg);
    return false;
  }
  if (isPrivate) {
    std::string msg = std::to_string(thread_id) + ": not cacheable because is private.";
    log.log(msg);
    return false;
  }
  if (expirationTime != "") {
    std::string msg =
        std::to_string(thread_id) + ": cacheable, expires at " + expirationTime;
    log.log(msg);
    return true;
  }
  if (maxAge != -1) {
    std::string msg =
        std::to_string(thread_id) + ": cacheable, expires at " + t.getTime(maxAge);
    log.log(msg);
    return true;
  }
  if (isPublic) {
    return true;
  }
  return false;
}

void ResponseInfo::logCat(int thread_id) {
  Logger log;
  if (maxAge != -1) {
    std::string msg = std::to_string(thread_id) +
                      ": NOTE Cache-Control: max-age=" + std::to_string(maxAge);
    log.log(msg);
  }
  if (expirationTime != "") {
    std::string msg = std::to_string(thread_id) + ": NOTE Expires: " + expirationTime;
    log.log(msg);
  }
  if (noCache) {
    std::string msg = std::to_string(thread_id) + ": NOTE Cache-Control:  no-cache";
    log.log(msg);
  }
  if (eTag != "") {
    std::string msg = std::to_string(thread_id) + ": NOTE ETag: " + eTag;
    log.log(msg);
  }
  if (lastModified != "") {
    std::string msg = std::to_string(thread_id) + ": NOTE Last-Modified: " + lastModified;
    log.log(msg);
  }
}

bool ResponseInfo::checkBadGateway(int client_fd, int thread_id) {
  Logger logFile;
  if (isBadGateway || response.find("\r\n\r\n") == std::string::npos) {
    std::string badGateway = "HTTP/1.1 502 Bad Gateway";
    std::string msg =
        std::to_string(thread_id) + ": Responding \"HTTP/1.1 502 Bad Gateway\"";
    logFile.log(msg);
    int status = send(client_fd, badGateway.c_str(), strlen(badGateway.c_str()), 0);
    if (status == -1) {
      return false;
    }
  }
  return true;
}
