#include "client.h"

#include "myexception.h"

/*
 * init socket and connect as a client, connect to a remote server
 * @param hostname
 * @param port
 * @return client_fd
 */
void Client::initClientfd(const char * hostname, const char * port) {
  struct addrinfo client_info;
  struct addrinfo * client_info_list;
  int status;
  // int client_fd;

  memset(&client_info, 0, sizeof(client_info));
  client_info.ai_family = AF_UNSPEC;
  client_info.ai_socktype = SOCK_STREAM;

  // initialize the client_info_list, which is a linked list of addrinfo structs
  status = getaddrinfo(hostname, port, &client_info, &client_info_list);
  if (status != 0) {
    string msg = "Error: cannot get address info for host: ";
    msg = msg + hostname + "\n";
    msg = msg + gai_strerror(status);
    throw myException(msg);
  }

  // socket_fd is the file descriptor for the socket that will listen for connections
  client_fd = socket(client_info_list->ai_family,
                     client_info_list->ai_socktype,
                     client_info_list->ai_protocol);
  if (client_fd == -1) {
    string msg = "Error: cannot create socket\n";
    msg = msg + "  (" + hostname + "," + port + ")";
    throw myException(msg);
  }

  // connect to the server
  status = connect(client_fd, client_info_list->ai_addr, client_info_list->ai_addrlen);
  if (status == -1) {
    string msg = "Error: cannot connect to socket\n";
    msg = msg + "  (" + hostname + "," + port + ")";
    throw myException(msg);
  }
  cout << "Connected to " << hostname << " on port " << port << endl;

  freeaddrinfo(client_info_list);
}
