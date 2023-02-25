#ifndef RESPONSEINFO_H
#define RESPONSEINFO_H

#include <string>
#include <vector>
// add header for boost::posix_time::ptime
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
using namespace std;

class ResponseInfo {
   public:
    string content;
    
    int status_code;
    string re_direct_url;

    // cache fields
    boost::posix_time::ptime expirationTime;
    string lastModified;
    string eTag;

    // for cache policy
    int cache_status;  // * not yet implemented *

    // for chunnk and content-length, ****havent discussed 403 yet****
    bool is_chunk;
    int content_length;
    string content_type;

    ResponseInfo() : is_chunk(false), content_length(-1){};

    void parseResponse(string & buffer) {
        setContentLength(buffer);
        setChunked(buffer);
        setStatusCode(buffer);
        checkStatus(buffer);
        setContentType(buffer)
    }

    ResponseInfo(string buffer) : is_chunk(false), content_length(-1) {

    }

    void setContentLength(string& buffer);
    void setChunked(string& buffer);
    void setStatusCode(string& buffer);
    void checkStatus(string& buffer);
    void setContentType(string& buffer);
};

#endif