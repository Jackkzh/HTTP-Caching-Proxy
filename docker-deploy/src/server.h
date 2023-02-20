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

  // server get what type of request it is from the browser, and print out the request
  void getRequest(char * client_request);

  void requestConnect(int id);

  // server get what type of request it is from the browser, and print out the request
  void getRequest(char * client_request);

 private:
  void connect_Transferdata(int send, int recv);
};
#endif