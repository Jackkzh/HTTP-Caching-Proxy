#ifndef CACHE_H
#define CACHE_H

#include <boost/date_time/posix_time/posix_time.hpp>

#include <map>
#include <mutex>
#include <string>

struct CacheItem {
  std::string content;
  boost::posix_time::ptime expirationTime;
  std::string lastModified = "";
  std::string eTag = "";
};

class Cache {
 private:
  std::map<std::string, CacheItem> cache;  // stores cached response for each URL
  int maxEntries;  // maximum number of entries allowed in the cache
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
  CacheItem get(std::string key);

  // caches the response for the given URL
  // the response will be removed from the cache after maxAge seconds
  void put(std::string key,
           std::string content,
           int maxAge,
           std::string lastModified,
           std::string eTag);

  // removes all entries from the cache
  void clear();

  bool validate(std::string key, std::string lastModified, std::string eTag);
};

#endif