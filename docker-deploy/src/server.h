#ifndef __SERVER__H__
#define __SERVER__H__
#include "helper.h"
//include <boost/beast/http.hpp>

class Server {
 public:
  int listen_fd;             // this is the fd for listening as a server
  int client_connection_fd;  // this is the fd for client connection after accept
  // const char * hostname;  // as a server, dont need to know the hostname
  const char * port;  // might not need to be a field, not sure(?)

  int initListenfd(const char * _port);
  void acceptConnection(string & ip);
  int getPort();

  void run();

  void requestConnect(int id);

  // server get what type of request it is from the browser, and print out the request
  static void getRequest(char * client_request, int id);
  static void * handleRequest(void * a);

 private:
  static void connect_Transferdata(int send, int recv);
};
#endif