#include "client.h"
#include "httpcommand.h"
#include "pthread.h"
#include "server.h"
// first creates and runs the server and listen for connections
// void run() {
//   Server server;
//   try {
//     server.initListenfd("12345");
//   }
//   catch (exception & e) {
//     std::cout << e.what() << std::endl;
//     return;
//   }
//   cout << "Server is listening on port: " << server.getPort() << endl;
//   string ip;
//   while (true) {
//     try {
//       server.acceptConnection(ip);
//     }
//     catch (exception & e) {
//       std::cout << e.what() << std::endl;
//       continue;
//     }

//     char browser_request[4096];
//     server.getRequest(browser_request);
//     // converrt the client_request to string
//     string client_request(browser_request);
//     httpcommand h(client_request);

//     // print out the h object
//     cout << "------" << endl;
//     cout << "Request: " << client_request << endl;
//     cout << "------" << endl;

//     // print size of httpcommand
//     cout << "size of httpcommand: " << sizeof(h) << endl;
//     h.printRequestInfo();

//     cout << "init connection with web server" << endl;
//     Client client;
//     try {
//       client.initClientfd(h.host.c_str(), h.port.c_str());
//     }
//     catch (exception & e) {
//       std::cout << e.what() << std::endl;
//       return;
//     }

//     send(client.client_fd, browser_request, strlen(browser_request), 0);
//     char buffer[40960];

//     int recv_len = 0;
//     while (true) {
//       ssize_t n = recv(client.client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
//       if (n == -1) {
//         perror("recv");
//         exit(EXIT_FAILURE);
//       }
//       else if (n == 0) {
//         break;
//       }
//       else {
//         recv_len += n;
//       }
//     }
//     cout << "recv_len: " << recv_len << endl;
//     // cout << "done sending to web server" << endl;
//     // int len_recv = recv(client_fd, buffer, 40960, 0);
//     // cout << "len_recv: " << len_recv << endl;
//     // cout << "using strlen" << strlen(buffer) << endl;  // this is wrong
//     cout << sizeof(buffer) << endl;
//     cout << "-------printing buffer from remote server-------" << endl;
//     cout << buffer << endl;
//     send(server.client_connection_fd, buffer, sizeof(buffer), 0);
//   }
// }

int main() {
  //string httpTest = "CONNECT server.example.com:90 HTTP/1.1\nHost: server.example.com:90";
  //httpcommand h_Test(httpTest);
  //h_Test.printRequestInfo();

  //  httpcommand h_Test(httpTest);
  //   h_Test.printRequestInfo();

  //   writeLog(httpTest);

  Server server;
  server.run();
  return EXIT_SUCCESS;
}