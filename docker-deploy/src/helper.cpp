#include "helper.h"
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

ClientInfo::ClientInfo(string uid, string ip, int fd, string arr) :
    uid(uid), ip(ip), fd(fd), arrivalTime(arr) {
}

void ClientInfo::addRequest(string req) {
  request = req;
}

void writeLog(string msg) {
  ofstream logFile(logFileLocation);
  pthread_mutex_lock(&mutex);
  logFile << msg << endl;
  pthread_mutex_unlock(&mutex);
}