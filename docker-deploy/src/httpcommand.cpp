#include "httpcommand.h"
/*
  * read the request string and parse the method, host, port and
  * url
  * @param req request string
*/
httpcommand::httpcommand(string req) : request(req) {
  parseMethod();
  parseHostPort();
  parseURL();
};
/*
  * request-line = method SP request-target SP HTTP-version CRLF
  * get method
*/
void httpcommand::parseMethod() {
  method = request.substr(0, request.find(" ", 0));
}

/*
  * Port: an optional port number in decimal following
  * the host and delimited from it by a single colon(":") character.
  * the "http" scheme defines a default port of "80"
  * 
  * request format could be: CONNECT server.example.com:80 HTTP/1.1
  *                          Host: server.example.com:80
  * 
  * or                       GET server.example.com HTTP/1.1
  *                          Host: server.example.com
  * get the host and port, while if there is not "Host: ", catch
  * an exception
*/

void httpcommand::parseHostPort() {
  string request_line = request.substr(0, request.find("\r\n", 0));
  try {
    size_t host_pos = request.find("Host: ", 0);
    //host_temp = server.example.com:80
    string host_temp =
        request.substr(host_pos + 6, request.find("\r\n", host_pos) - 6 - host_pos);
    size_t port_pos = host_temp.find(":", 0);
    if (port_pos != std::string::npos) {
      host = host_temp.substr(0, port_pos);
      port = host_temp.substr(port_pos + 1);
    }
    else {
      host = host_temp;
      port = "80";
    }
  }
  catch (exception & e) {
    cerr << "Error: Empty Host." << endl;
    exit(EXIT_FAILURE);
  }
}

/*
  * get whole url
*/
void httpcommand::parseURL() {
  int url_pos = request.find(" ", 0);
  int url_pos2 = request.find(" ", url_pos + 1);
  url = request.substr(url_pos + 1, url_pos2 - url_pos - 1);
}