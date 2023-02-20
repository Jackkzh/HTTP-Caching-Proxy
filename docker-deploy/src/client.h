#ifndef __CLIENT__H__
#define __CLIENT__H__
#include "helper.h"

class Client {
 public:
  int client_fd;
  int client_connection_fd;

  Client(){};
  Client(int fd) : client_connection_fd(fd){};
  void initClientfd(const char * hostname, const char * port);
};
#endif