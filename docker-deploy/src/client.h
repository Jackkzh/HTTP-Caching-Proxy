#ifndef __CLIENT__H__
#define __CLIENT__H__
#include "helper.h"
#include "ResponseInfo.h"

class Client {
 public:
  int client_fd;
  int client_connection_fd;
  ResponseInfo response_info;

  Client() {};
  Client(string buffer) : response_info(buffer) {};
  Client(int fd1, int fd2, string buffer) : client_fd(fd1), client_connection_fd(fd2), response_info(buffer) {};
  void initClientfd(const char * hostname, const char * port);
};
#endif