#include "httpcommand.h"

httpcommand::httpcommand() :
    request(""), method(""), port(""), host(""), url(""), isBadRequest(false) {
}
/**
  * read the request string and parse the method, host, port and
  * url
  * @param req request string
*/
httpcommand::httpcommand(std::string req) : request(req) {
  std::cout << req << std::endl;
  parseMethod();
  parseHostPort();
  parseURL();
  parseValidInfo();
};

void httpcommand::printRequestInfo() {
  std::cout << "------" << std::endl;
  std::cout << "Request: " << request << std::endl;
  std::cout << "------" << std::endl;

  std::cout << "Method: " << method << std::endl;
  std::cout << "Path: " << url << std::endl;
  std::cout << "Port: " << port << std::endl;
  std::cout << "Host: " << host << std::endl;
  std::cout << "------------------" << std::endl;
}
/**
  * request-line   = method SP request-target SP HTTP-version CRLF
  * get method
*/
void httpcommand::parseMethod() {
  if (request.size() == 0) {
    // handle the error
    isBadRequest = true;
    return;
  }
  method = request.substr(0, request.find(" ", 0));
}

/**
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
  std::string request_line = request.substr(0, request.find("\r\n", 0));
  size_t host_pos = request.find("Host: ", 0);
  if (host_pos != std::string::npos) {
    // host_temp = server.example.com:80
    std::string host_temp =
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
  else {
    isBadRequest = true;
  }
}

/**
  * get whole url
*/
void httpcommand::parseURL() {
  int url_pos = request.find(" ", 0);
  int url_pos2 = request.find(" ", url_pos + 1);
  if (url_pos != std::string::npos && url_pos2 != std::string::npos) {
    url = request.substr(url_pos + 1, url_pos2 - url_pos - 1);
  }
  else {
    isBadRequest = true;
  }
}

void httpcommand::parseValidInfo() {
  ifModifiedSince = "";
  ifNoneMatch = "";
}

bool httpcommand::checkBadRequest(int client_fd, int thread_id) {
  Logger logFile;
  if (isBadRequest || (method != "CONNECT" && method != "POST" && method != "GET")) {
    std::string badRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";
    std::string msg =
        std::to_string(thread_id) + ": Responding \"HTTP/1.1 400 Bad Request\"";
    logFile.log(msg);
    int status = send(client_fd, badRequest.c_str(), strlen(badRequest.c_str()), 0);
    if (status == -1) {
      return false;
    }
  }
  return true;
}

bool checkBadGateway(std::string str, int client_fd, int thread_id) {
  Logger logFile;
  if (str.find("\r\n\r\n") == std::string::npos) {
    std::string badGateway = "HTTP/1.1 502 Bad Gateway";
    std::string msg =
        std::to_string(thread_id) + ": Responding \"HTTP/1.1 502 Bad Gateway\"";
    logFile.log(msg);
    int status = send(client_fd, badGateway.c_str(), strlen(badGateway.c_str()), 0);
    if (status == -1) {
      return false;
    }
  }
  return true;
}