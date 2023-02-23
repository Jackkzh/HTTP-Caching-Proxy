#ifndef __SERVER__H__
#define __SERVER__H__
#include <signal.h>

#include <mutex>
#include <unordered_map>

#include "helper.h"
class Server {
 public:
  int listen_fd;             // this is the fd for listening as a server
  int client_connection_fd;  // this is the fd for client connection after accept
  // const char * hostname;  // as a server, dont need to know the hostname
  const char * port;  // might not need to be a field, not sure(?)
  Server(){};
  void run(int thread_id);

  // // server get what type of request it is from the browser, and print out the request
  // void getRequest(char * client_request, int id);
  // void handleRequest();

  int initListenfd(const char * _port);
  void acceptConnection(string & ip);
  int getPort();

 private:
  std::mutex mtx;
  string client_ip;
  void handleRequest(int thread_id);
  void requestCONNECT(int client_fd, int thread_id);
  void connect_Transferdata(int fd1, int fd2);
  void writeLog(string msg);
};
#endif