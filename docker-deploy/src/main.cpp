#include <thread>

#include "client.h"
#include "httpcommand.h"
#include "proxy.h"
#include "server.h"

void test(Proxy server, int thread_id) {
  server.run(thread_id);
}

int main() {
  Proxy server;
  try {
    server.initListenfd("12345");
  }
  catch (exception & e) {
    std::cout << e.what() << std::endl;
  }
  cout << "Server is listening on port: " << server.getPort() << endl;

  string ip;
  int thread_id = 0;
  while (true) {
    thread_id++;
    try {
      server.acceptConnection(ip);
      /*   --------- for testing multithread ---------   */
      // cout << "the thread id: " << thread_id << endl;
      // cout << "my fd: " << server.socket_fd << endl;
      // cout << "client connection id : " << server.client_connection_fd << endl;
      /*   --------- for testing multithread ---------   */
    }
    catch (exception & e) {
      std::cout << e.what() << std::endl;
      continue;
    }
    thread th(test, server, thread_id);
    th.detach();
  }
  return EXIT_SUCCESS;
}