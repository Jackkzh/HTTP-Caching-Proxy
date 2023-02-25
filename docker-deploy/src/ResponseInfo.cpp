#include "ResponseInfo.h"

/**
 * Set the content length of the HTTP response
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::setContentLength(std::string & buffer) {
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

/**
 * Check the status code of the HTTP response and take action
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::checkStatus(std::string & buffer) {
  if (status_code == 200) {
    // do nothing
  }
  else if (status_code == 301) {
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
  else if (status_code == 304) {
    // do nothing
  }
  else if (status_code == 403) {
    // do nothing
  }
  else if (status_code == 404) {
    // do nothing
  }
  else if (status_code == 500) {
    // do nothing
  }
  else {
    // do nothing
  }
}

/**
 * Set cache control fields based on the "Cache-Control",
 * "Expires", "Last-Modified", and "ETag" headers in the HTTP response.
 * @param buffer the buffer containing the HTTP response
 */
void ResponseInfo::setCacheControl(std::string & buffer) {
  std::size_t end = buffer.find("\r\n\r\n");
  if (end != std::string::npos) {
    std::string headers = buffer.substr(0, end);
    // use Boost to find if the header contains "Cache-Control: "
    boost::regex re("Cache-Control:\\s*(\\S+)");
    boost::smatch what;
    if (boost::regex_search(headers, what, re)) {
      // get the string after "Cache-Control: "
      std::string cache_control = what[1].str();

      // check if cache-control contains "no-cache" directive
      if (cache_control.find("no-cache") != std::string::npos) {
        noCache = true;
      }

      // use Boost to find if the header contains "max-age=seconds"
      re = boost::regex("max-age=(\\d+)");
      if (boost::regex_search(cache_control, what, re)) {
        maxAge = std::stoi(what[1].str());
      }

      // use Boost to find if the header contains "Expires: "
      re = boost::regex("Expires:\\s*([^\r\n]*)");
      if (boost::regex_search(headers, what, re)) {
        expirationTime = what[1].str();
        // std::cout << "expirationTime: " << expirationTime << std::endl;
      }

      // use Boost to find if the header contains "Last-Modified: "
      re = boost::regex("Last-Modified:\\s*([^\r\n]*)");
      if (boost::regex_search(headers, what, re)) {
        lastModified = what[1].str();
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
  }
}

/**
 * Print the cache fields of the HTTP response 
 * maxAge, noCache, expirationTime, lastModified, eTag.
 */
void ResponseInfo::printCacheFields() {
  std::cout << "maxAge: " << maxAge << std::endl;
  std::cout << "noCache: " << noCache << std::endl;
  std::cout << "expirationTime: " << expirationTime << std::endl;
  std::cout << "lastModified: " << lastModified << std::endl;
  std::cout << "eTag: " << eTag << std::endl;
}