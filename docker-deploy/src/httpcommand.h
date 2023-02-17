#ifndef __HTTPCOMMAND__H__
#define __HTTPCOMMAND__H__
#include <exception>
#include <iostream>
#include <string>

using namespace std;

class httpcommand {
 public:
  string request;
  string method;
  string port;
  string host;
  string url;

 public:
  httpcommand(string req);

 private:
  void parseMethod();
  void parseHostPort();
  void parseURL();
};
#endif