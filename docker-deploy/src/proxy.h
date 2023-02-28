#ifndef __PROXY__H__
#define __PROXY__H__
#include <boost/regex.hpp>

#include "helper.h"
#include "httpcommand.h"
#include "myexception.h"

class Proxy {
 public:
  int socket_fd;             // this is the listen socket_fd
  int client_connection_fd;  // socket fd on the server side after accept

 private:
  std::string client_ip;  // the ip address of the client

 public:
  Proxy() {}
  Proxy(int socket_fd, int client_connection_fd) :
      socket_fd(socket_fd), client_connection_fd(client_connection_fd) {}

  void initListenfd(const char * port);
  int build_connection(const char * host, const char * port);
  void acceptConnection(std::string & ip);
  int getPort();
  void requestCONNECT(int client_fd, int thread_id);
  void connect_Transferdata(int recv_fd, int send_fd);
  void handleRequest(int thread_id);
  void requestGET(int client_fd, httpcommand request, int thread_id);
  void run(int thread_id);
  void requestPOST(int client_fd, httpcommand request_info, int thread_id);
  void sendChunkPacket(int remoteFd, int client_connection_fd);
};
#endif