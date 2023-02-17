#include "httpcommand.h"
#include "server.h"

int main() {
  string httpTest = "CONNECT server.example.com:90 HTTP/1.1\nHost: server.example.com:90";
  httpcommand h_Test(httpTest);
  cout << h_Test.method << endl;
  cout << h_Test.port << endl;
  cout << h_Test.host << endl;
  cout << h_Test.url << endl;
}