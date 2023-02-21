#ifndef __HTTPCOMMAND__H__
#define __HTTPCOMMAND__H__
#include "helper.h"

using namespace std;

class httpcommand {
 public:
  string request;
  string method;
  string port;
  string host;
  string url;

 public:
  httpcommand();
  httpcommand(string req);
  void printRequestInfo();

 private:
  void parseMethod();
  void parseHostPort();
  void parseURL();
};
#endif