#include "helper.h"
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

ClientInfo::ClientInfo(int uid, string ip, int fd, string arr) :
    uid(uid), ip(ip), fd(fd), arrivalTime(arr) {
}

void ClientInfo::addRequest(string req) {
  request = req;
}

string TimeMake::getTime() {
  time_t currTime = time(0);
  struct tm * localTime = localtime(&currTime);
  const char * t = asctime(localTime);
  return string(t);
}

/**
 * handler message body, including header and body
 * @param len 
 * @param req
 * @param idx
 * @param body
 * @return bool
*/
bool messageBodyHandler(int len, string req, int & idx, bool & body) {
  int endPos, contLen = 0;
  if ((endPos = req.find("\r\n\r\n")) != string::npos) {
    int contentPos = req.find("Content-Length: ", 0);
    if (contentPos == string::npos) {
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

/**
 * check if the request is malformed, send 400 Bad Request if it is
 * @param req
 * @param client_fd
 * @return bool
*/
bool checkBadRequest(string req, int client_fd) {
  if (req.find("Host:", 0) == string::npos) {
    string badRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";
    int status = send(client_fd, badRequest.c_str(), strlen(badRequest.c_str()), 0);
    if (status == -1) {
      return false;
    }
  }
  return true;
}