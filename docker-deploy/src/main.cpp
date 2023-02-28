#include <thread>

#include "ResponseInfo.h"
#include "cache.h"
#include "httpcommand.h"
#include "proxy.h"

void test(Proxy server, int thread_id) {
    server.run(thread_id);
}

/*
void testCache() {
  Cache cache(10 * 1024 * 1024);
  // Add a response to the cache
  std::string url = "http://example.com/page1";
  std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n0123456789";
  int max_Age = 10000000;
  std::string lastmodified = "Fri, 09 Aug 2013 23:54:35 GMT";
  std::string etag = "1541025663";
  cache.put(url, response, max_Age, lastmodified, etag);
  // Check if the response is in the cache
  if (cache.has(url)) {
    // Get the response from the cache
    std::cout << "----------content--------------" << std::endl;
    std::cout << "Cached response: \n" << cache.get(url).content << std::endl;
    std::cout << "----------cache items--------------" << std::endl;
    std::cout << "expirationTime: " << cache.get(url).expirationTime << std::endl;
    std::cout << "lastModified: " << cache.get(url).lastModified << std::endl;
    std::cout << "eTag: " << cache.get(url).eTag << std::endl;
  }
  else {
    std::cout << "Response not in cache" << std::endl;
  }
  std::cout << "\n\n----------after update--------------\n\n" << std::endl;
  //update
  std::string lastmodified2 = "Fri, 15 Aug 2022 20:13:35 GMT";
  std::string etag2 = "150000000";
  cache.put(url, response, max_Age, lastmodified2, etag2);
  // Check if the response is in the cache
  std::cout << "\n\nCheck if the response is in the cache\n\n" << std::endl;
  if (cache.has(url)) {
    // Get the response from the cache
    std::cout << "----------content--------------" << std::endl;
    std::cout << "Cached response: \n" << cache.get(url).content << std::endl;
    std::cout << "----------cache items--------------" << std::endl;
    std::cout << "expirationTime: " << cache.get(url).expirationTime << std::endl;
    std::cout << "lastModified: " << cache.get(url).lastModified << std::endl;
    std::cout << "eTag: " << cache.get(url).eTag << std::endl;
  }
  else {
    std::cout << "Response not in cache" << std::endl;
  }
  std::cout << "\n\n\n" << std::endl;
  std::cout << "----------Test Validate--------------\n" << std::endl;
  if (response.find("200")) {
    if (cache.has(url)) {
      std::string lastmodified3 = "Fri, 16 Aug 2022 20:13:35 GMT";
      std::string etag3 = "160000000";
      std::cout << "When Failed to validate, we will print: \n" << std::endl;
      if (!cache.validate(url, lastmodified3, etag3)) {
        std::cout << "----------Original content--------------" << std::endl;
        std::cout << "expirationTime: " << cache.get(url).expirationTime << std::endl;
        std::cout << "lastModified: " << cache.get(url).lastModified << std::endl;
        std::cout << "eTag: " << cache.get(url).eTag << std::endl;
        std::cout << "\nBut excepted:\n" << std::endl;
        std::cout << "lastModified: " << lastmodified3 << std::endl;
        std::cout << "eTag: " << etag3 << std::endl;
        std::cout << "\nFailed, update cache!\n" << std::endl;
        cache.put(url, response, max_Age, lastmodified3, etag3);
        std::cout << "After update:\n" << std::endl;
        std::cout << "----------content--------------" << std::endl;
        std::cout << "Cached response: \n" << cache.get(url).content << std::endl;
        std::cout << "----------cache items--------------" << std::endl;
        std::cout << "expirationTime: " << cache.get(url).expirationTime << std::endl;
        std::cout << "lastModified: " << cache.get(url).lastModified << std::endl;
        std::cout << "eTag: " << cache.get(url).eTag << std::endl;
        //then handle it
      }
    }
    else {
      std::cout << "When Success to validate, we will print: \n" << std::endl;
      std::cout << "----------content--------------" << std::endl;
      std::cout << "Cached response: \n" << cache.get(url).content << std::endl;
      std::cout << "----------cache items--------------" << std::endl;
      std::cout << "expirationTime: " << cache.get(url).expirationTime << std::endl;
      std::cout << "lastModified: " << cache.get(url).lastModified << std::endl;
      std::cout << "eTag: " << cache.get(url).eTag << std::endl;
    }
  }
  std::cout << "\n\n----------Test Expiration Time--------------\n" << std::endl;
  Cache cache2(10);
  std::string key = "example.com";
  std::string content = "Hello, World!";
  int maxAge = 1;
  cache2.put(key, content, maxAge, "", "");
  // Wait for the cache item to expire
  std::this_thread::sleep_for(std::chrono::seconds(maxAge + 1));
  // Try to retrieve the item from the cache
  // Call validate with the same key and arbitrary ETag and last modified values.
  std::string eTag = "arbitraryETag";
  std::string lastModified = "arbitraryLastModified";
  if (cache.validate(key, lastModified, eTag)) {
    std::cout << "The cache has the item and it has not expired." << std::endl;
  }
  else {
    std::cout << "The cache does not have the item or it has expired." << std::endl;
  }
  cache.clear();
}
*/

void cleanfile() {
    std::ofstream ofs(logFileLocation, std::ios::out | std::ios::trunc);
    ofs.close();
}

void testResponse() {
    std::string buffer =
        "HTTP/1.1 200 OK\r\n"
        "Date: Thu, 22 Feb 2023 12:34:56 GMT\r\n"
        "Cache-Control: max-age=3600, public, must-revalidate\r\n"
        "Expires: Thu, 22 Feb 2023 13:34:56 GMT\r\n"
        "Last-Modified: Wed, 21 Feb 2023 12:34:56 GMT\r\n"
        "ETag: \"123456789\"\r\n"
        "Content-Length: 42\r\n"
        "\r\n"
        "Hello, world! This is a test.";
    TimeMake t;
    ResponseInfo response;
    response.parseResponse(buffer, t.getTime());
    response.printCacheFields();
}

int main() {
    cleanfile();

    Proxy server;
    try {
        server.initListenfd("12345");
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    std::cout << "Server is listening on port: " << server.getPort() << std::endl;

    std::string ip;
    int thread_id = 0;
    while (true) {
        thread_id++;
        try {
            server.acceptConnection(ip);
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            continue;
        }
        std::thread th(test, server, thread_id);
        th.detach();
    }
    return EXIT_SUCCESS;
}