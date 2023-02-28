#ifndef CACHE_H
#define CACHE_H

#include <map>
#include <mutex>
#include <string>

#include "ResponseInfo.h"
#include "helper.h"
#include "httpcommand.h"

class Cache {
 private:
  std::map<std::string, ResponseInfo> cache;  // stores cached response for each URL
  int maxEntries;  // maximum number of entries allowed in the cache
  std::mutex cacheMutex;
  void cleanup();  // helper function to remove expired entries from the cache

 public:
  Cache(int maxEntries);
  ~Cache();
  // checks whether the cache contains a cached response for the given URL
  bool has(std::string key);

  // returns the cached response for the given URL
  ResponseInfo get(std::string key);

  // caches the response for the given URL
  // the response will be removed from the cache after maxAge seconds
  void put(std::string key, ResponseInfo response);

  // removes all entries from the cache
  void clear();

  bool validate(std::string key, std::string & request);

  void printCache();

  void useCache(httpcommand req, int client_fd, int thread_id);
};

#endif