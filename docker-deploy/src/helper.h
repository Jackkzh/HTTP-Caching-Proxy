#ifndef __HELPER__H__
#define __HELPER__H__
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
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

class Logger {
 private:
  std::ofstream log_file;
  std::mutex mtx;

 public:
  Logger() {
    // Check if log file exists and truncate it if it does
    if (FILE * file = fopen(logFileLocation, "r")) {
      fclose(file);
      std::ofstream(logFileLocation, std::ios::trunc);
    }
    log_file.open(
        logFileLocation,
        std::ios::out | std::ios::app);  // ios::app is a flag used to append the file
    if (!log_file) {
      throw std::runtime_error("Failed to open log file");
    }
  }

  void log(const std::string & message) {
    std::lock_guard<std::mutex> lock{mtx};
    log_file << message << std::endl;
  }
};

class ClientInfo {
 public:
  std::string request;
  int uid;                  //unique identifier for the client request
  std::string ip;           //IP address that client made request from
  int fd;                   //file descriptor
  std::string arrivalTime;  //the time when the request arrived

  ClientInfo(){};
  ClientInfo(int uid, std::string ip, int fd, std::string arr);
  void addRequest(std::string req);
};

class TimeMake {
 public:
  std::string getTime();
};

bool messageBodyHandler(int len, std::string req, int & idx, bool & body);
bool checkBadRequest(std::string req, int client_fd);
#endif