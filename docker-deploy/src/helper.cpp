#include "helper.h"
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

ClientInfo::ClientInfo(string uid, string ip, int fd, string arr) :
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