#ifndef __HELPER__H__
#define __HELPER__H__
#include <arpa/inet.h>
#include <assert.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "httpcommand.h"
#define MAX_LENGTH 65536
#define logFileLocation "./proxy.log"  //"/var/log/erss/proxy.log"

class Logger {
 private:
  std::ofstream log_file;
  std::mutex mtx;

 public:
  Logger() {
    log_file.open(
        logFileLocation,
        std::ios::out | std::ios::app);  // ios::app is a flag used to append the file
    if (!log_file) {
      throw std::runtime_error("Failed to open log file");
    }
  }

  void log(std::string message) {
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
  std::map<std::string, int> weekMap;
  std::map<std::string, int> monthMap;

  TimeMake() {
    std::string weekday_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    std::string month_names[] = {"Jan",
                                 "Feb",
                                 "Mar",
                                 "Apr",
                                 "May",
                                 "Jun",
                                 "Jul",
                                 "Aug",
                                 "Sep",
                                 "Oct",
                                 "Nov",
                                 "Dec"};

    for (int i = 0; i < 7; i++) {
      weekMap.insert(std::pair<std::string, int>(weekday_names[i], i));
    }
    for (int i = 0; i < 12; i++) {
      monthMap.insert(std::pair<std::string, int>(month_names[i], i));
    }
  }
  int getweekMap(std::string str) { return weekMap.find(str)->second; }
  int getmonthMap(std::string str) { return monthMap.find(str)->second; }
  std::string getTime(int s = 0);
  std::string convertGMT(std::string timeGMT);
  int timeMinus(std::string timeStr1, std::string timeStr2);
};

bool messageBodyHandler(int len, std::string req, int & idx, bool & body);
bool checkBadRequest(httpcommand req, int client_fd, int thread_id);
bool checkBadGateway(std::string str, int client_fd, int thread_id);
#endif