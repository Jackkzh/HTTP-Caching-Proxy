#ifndef __HTTPCOMMAND__H__
#define __HTTPCOMMAND__H__
#include "helper.h"
#include "myexception.h"

class httpcommand {
 public:
  std::string request;
  std::string method;
  std::string port;
  std::string host;
  std::string url;
  std::string ifNoneMatch;
  std::string ifModifiedSince;

 public:
  httpcommand();
  httpcommand(std::string req);
  void printRequestInfo();

 private:
  void parseMethod();
  void parseHostPort();
  void parseURL();
  void parseValidInfo();
};
bool checkBadRequest(httpcommand req, int client_fd, int thread_id);
bool checkBadGateway(std::string str, int client_fd, int thread_id);

#endif