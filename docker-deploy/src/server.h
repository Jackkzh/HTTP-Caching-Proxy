#ifndef __SERVER__H__
#define __SERVER__H__
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;
ofstream log("/var/log/erss/proxy.log");

class Server {
 public:
  int status;
  int socket_fd;
  int client_connection_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  const char * hostname;
  const char * port;

  void initStatus(const char * _hostname, const char * _port);
  void createSocket();
  int acceptConnection(string & ip);
  int getPort();
  void requestConnect(int id);

 private:
  void connect_Transferdata(int send, int recv);
};
#endif