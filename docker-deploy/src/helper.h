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
#include <mutex>
#include <string>
#include <vector>

#define MAX_LENGTH 65536
#define logFileLocation "./proxy.log"  //"/var/log/erss/proxy.log"

using namespace std;

class Logger {
 private:
  ofstream log_file;
  mutex mtx;

 public:
  Logger() {
    // Check if log file exists and truncate it if it does
    if (FILE * file = fopen(logFileLocation, "r")) {
      fclose(file);
      ofstream(logFileLocation, ios::trunc);
    }
    log_file.open(logFileLocation,
                  ios::out | ios::app);  // ios::app is a flag used to append the file
    if (!log_file) {
      throw runtime_error("Failed to open log file");
    }
  }

  void log(const string & message) {
    lock_guard<mutex> lock{mtx};
    log_file << message << endl;
  }
};

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
bool checkBadRequest(string req, int client_fd);
#endif