#ifndef CACHE_H
#define CACHE_H

#include <boost/date_time.hpp>
#include <map>
#include <mutex>
#include <string>

#include "ResponseInfo.h"
#include "helper.h"
#include "httpcommand.h"

/*
Example:
{
  "content": "Hello world!", // everything after headers
  "expirationTime": "Thu, 22 Feb 2023 13:34:56 GMT",
  "lastModified": "Thu, 22 Feb 2023 13:34:56 GMT",
  "eTag": "1512463465"
}
*/

struct CacheItem {
    std::string content;
    std::string expirationTime;
    std::string lastModified = "";
    std::string eTag = "";
};

class Cache {
   private:
    std::map<std::string, ResponseInfo> cache;  // stores cached response for each URL
    int maxEntries;                             // maximum number of entries allowed in the cache
    std::mutex cacheMutex;
    void cleanup();  // helper function to remove expired entries from the cache

   public:
    Cache(int maxEntries);
    ~Cache();
    // checks whether the cache contains a cached response for the given URL
    // returns true if the cache contains a response for the given URL, false otherwise
    bool has(std::string key);

    // returns the cached response for the given URL
    // throws std::out_of_range if the cache does not contain a response for the given URL
    ResponseInfo get(std::string key);

    // caches the response for the given URL
    // the response will be removed from the cache after maxAge seconds
    void put(std::string key, ResponseInfo response);

    // removes all entries from the cache
    void clear();

    bool validate(std::string key, std::string& request);

    bool isFresh();

    void printCache();

    void useCache(httpcommand req, int client_fd, int thread_id);
};

#endif