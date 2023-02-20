#ifndef __CLIENT__H__
#define __CLIENT__H__
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
using namespace std;

class Client {
 public:
  int client_fd;

  void initClientfd(const char * hostname, const char * port);
};
#endif