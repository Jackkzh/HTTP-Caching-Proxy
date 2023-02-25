#include "cache.h"

Cache::Cache(int maxEntries) {
  this->maxEntries = maxEntries;
}

Cache::~Cache() {
}

/**
 * This function checks whether the cache contains a cached response 
 * for the given URL.
 * @param key representing the URL.
 * @return If the cache contains a response for the given URL, 
 * the function returns true. Otherwise, it returns false.
*/
bool Cache::has(std::string key) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  cleanup();
  if (cache.count(key) > 0) {
    return true;
  }
  else {
    return false;
  }
}

/**
   * This function retrieves a cached response for the given URL.
   * @param key representing the URL.
   * @return The cached response for the given URL.
   */
CacheItem Cache::get(std::string key) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  cleanup();
  return cache.at(key);
}

/**
   * This function adds a new cached response for the given URL.
   * @param key representing the URL.
   * @param content the response content to be cached.
   * @param maxAge the maximum age of the cached response in seconds.
   * @param lastModified the last modified time of the response, if available.
   * @param eTag the ETag of the response, if available.
   */
void Cache::put(std::string key,
                std::string content,
                int maxAge,
                std::string lastModified,
                std::string eTag) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  cleanup();

  boost::posix_time::ptime expirationTime =
      boost::posix_time::second_clock::local_time() + boost::posix_time::seconds(maxAge);

  if (cache.size() >= maxEntries) {
    cache.erase(cache.begin());
  }
  CacheItem item = {content, expirationTime, lastModified, eTag};

  if (lastModified != "") {
    item.lastModified = lastModified;
  }
  if (eTag != "") {
    item.eTag = eTag;
  }

  cache[key] = item;
}

void Cache::clear() {
  std::lock_guard<std::mutex> lock(cacheMutex);
  cache.clear();
}

void Cache::cleanup() {
  boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
  std::vector<std::string> expiredKeys;

  for (auto const & item : cache) {
    if (item.second.expirationTime < now) {
      expiredKeys.push_back(item.first);
    }
  }

  for (auto const & key : expiredKeys) {
    cache.erase(key);
  }
}

/**
 * Validates the cached response for the given key based on the response headers.
 * If the cached response is still valid, the method returns true.
 * Otherwise, it returns false and removes the cached response for the given key.
 * 
 * @param key The key for the cached response.
 * @param lastModified the last modified time of the response, if available.
 * @param eTag the ETag of the response, if available.
 * @return True if the cached response is still valid, false otherwise.
 */
bool Cache::validate(std::string key, std::string lastModified, std::string eTag) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  cleanup();
  // Check if the cache has the requested URL
  if (cache.count(key) == 0) {
    return false;
  }

  CacheItem cachedItem = cache.at(key);

  if (cachedItem.expirationTime < boost::posix_time::second_clock::local_time()) {
    // Cached response has expired.
    return false;
  }
  if (eTag != "" && eTag != cachedItem.eTag) {
    // ETag mismatch.
    return false;
  }

  if (lastModified != "" && lastModified != cachedItem.lastModified) {
    // Last-Modified time mismatch.
    return false;
  }
  return true;
}