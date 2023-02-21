#ifndef __HELPER__H__
#define __HELPER__H__
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define MAX_LENGTH 65536
#define logFileLocation "./proxy.log"  //"/var/log/erss/proxy.log"

using namespace std;

class ClientInfo {
 public:
  string request;
  int uid;             //unique identifier for the client request
  string ip;           //IP address that client made request from
  int fd;              //file descriptor
  string arrivalTime;  //the time when the request arrived

  ClientInfo(){};
  ClientInfo(int uid, string ip, int fd, string arr);
  void addRequest(string req);
};

class TimeMake {
 public:
  string getTime();
};

bool messageBodyHandler(int len, string req, int & idx, bool & body);

#endif