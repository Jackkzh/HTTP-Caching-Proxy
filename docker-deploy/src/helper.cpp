#include "helper.h"
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

ClientInfo::ClientInfo(string uid, string ip, int fd, string arr) :
    uid(uid), ip(ip), fd(fd), arrivalTime(arr) {
}

void ClientInfo::addRequest(string req) {
  request = req;
}

ofstream LogFile::logFile;

LogFile::LogFile() {
  logFile.open(logFileLocation);
  if (logFile.is_open()) {
    return;
  }
  ofstream logFile(logFileLocation);
  logFile.open(logFileLocation);
  assert(logFile.is_open());
}

void LogFile::writeLog(string msg) {
  pthread_mutex_lock(&mutex);
  logFile << msg << endl;
  pthread_mutex_unlock(&mutex);
}

string TimeMake::getTime() {
  time_t currTime = time(0);
  struct tm * localTime = localtime(&currTime);
  const char * t = asctime(localTime);
  return string(t);
}