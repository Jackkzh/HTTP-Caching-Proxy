#ifndef __HTTPCOMMAND__H__
#define __HTTPCOMMAND__H__
#include <string>

using namespace std;

class httpcommand{
public:
    string request;
    string method;
    string port;
    string host;
    string url;
public:
    httpcommand(){};
    void parseRequest(String req);
private:
    void parseMethod();
    void parsePort();
    void parseHost();
    void parseURL();
}