#ifndef __PROXY__H__
#define __PROXY__H__
#include <mutex>

#include "client.h"
#include "helper.h"
#include "httpcommand.h"
#include "myexception.h"


// extern std::mutex mtx;
// extern ofstream logFile(logFileLocation);

class Proxy {
  public:
  int socket_fd;              // this is the listen socket_fd
  int client_connection_fd;   // socket fd on the server side after accept

 private:
  string client_ip;           // the ip address of the client

 public:
  Proxy() {}
  Proxy(int socket_fd, int client_connection_fd) :
      socket_fd(socket_fd), client_connection_fd(client_connection_fd) {}

  void initListenfd(const char * port);
  int build_connection(string host, string port);
  void acceptConnection(string & ip);
  int getPort();
  void requestCONNECT(int client_fd, int thread_id);
  void connect_Transferdata(int recv_fd, int send_fd);
  void handleRequest(int thread_id);
  void requestGET(int client_fd, httpcommand h, int thread_id);
  void run(int thread_id);
};
#endif