#include "helper.h"
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

ClientInfo::ClientInfo(int uid, std::string ip, int fd, std::string arr) :
    uid(uid), ip(ip), fd(fd), arrivalTime(arr) {
}

void ClientInfo::addRequest(std::string req) {
  request = req;
}

std::string TimeMake::getTime(int s) {
  time_t currTime = time(0) + s;
  struct tm * localTime = localtime(&currTime);
  const char * t = asctime(localTime);
  return std::string(t).substr(0, std::string(t).size() - 1);
}

std::string TimeMake::convertGMT(std::string timeGMT) {
  std::tm tmStruct;
  strptime(timeGMT.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tmStruct);

  char outputFormat[30];
  sprintf(outputFormat,
          "%s %s %d %02d:%02d:%02d %d",
          timeGMT.substr(0, 3).c_str(),
          timeGMT.substr(8, 3).c_str(),
          tmStruct.tm_mday,
          tmStruct.tm_hour,
          tmStruct.tm_min,
          tmStruct.tm_sec,
          tmStruct.tm_year + 1900);

  return std::string(outputFormat);
}

int TimeMake::timeMinus(std::string timeStr1, std::string timeStr2) {
  std::tm tm1 = {};
  strptime(timeStr1.c_str(), "%a %b %d %H:%M:%S %Y", &tm1);
  std::time_t time1 = std::mktime(&tm1);

  std::tm tm2 = {};
  strptime(timeStr2.c_str(), "%a %b %d %H:%M:%S %Y", &tm2);
  std::time_t time2 = std::mktime(&tm2);

  return std::difftime(time1, time2);
}

/**
 * handler message body, including header and body
 * @param len 
 * @param req
 * @param idx
 * @param body
 * @return bool
*/
bool messageBodyHandler(int len, std::string req, int & idx, bool & body) {
  int endPos, contLen = 0;
  if ((endPos = req.find("\r\n\r\n")) != std::string::npos) {
    int contentPos = req.find("Content-Length: ", 0);
    if (contentPos == std::string::npos) {
      return false;
    }
    int end = req.find("\r\n", contentPos);
    if (body) {
      contLen = stoi(req.substr(contentPos + 16, end - (contentPos + 16)));
      if (idx - endPos - 4 >= contLen) {
        return false;
      }
      else {
        contLen = contLen + endPos - idx + 4;
        contLen += len;
      }
      body = false;
    }
    else {
      contLen -= len;
      if (contLen <= 0) {  // invaild content-Length field value
        return false;
      }
    }
  }
  return true;
}
