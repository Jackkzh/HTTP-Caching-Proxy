#include "server.h"

#include "httpcommand.h"
#include "myexception.h"

/*
 * init socket status, bind socket, listen on socket as a server(to accept connection from browser)
 * @param _port
 * @return listen_fd
 */
int Server::initListenfd(const char * _port) {
  port = _port;
  int status;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, port, &host_info, &host_info_list);
  // const char * hostname
  struct sockaddr_in * sa = (struct sockaddr_in *)host_info_list->ai_addr;
  const char * hostname = inet_ntoa(sa->sin_addr);
  cout << "hostname: " << hostname << endl;  // test if hostname is correct

  char ip4[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(sa->sin_addr), ip4, INET_ADDRSTRLEN);
  string ip4_str(ip4);
  cout << "ip4 in char[]: " << ip4 << endl;
  cout << "ip4_str: " << ip4_str << endl;
  // using ntop to convert ip address to hostname

  if (status != 0) {
    string msg = "Error: cannot get address info for host\n";
    msg = msg + "  (" + hostname + "," + port + ")";
    throw myException(msg);
  }

  if (port == NULL) {
    // OS will assign a port
    struct sockaddr_in * addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
    addr_in->sin_port = 0;
  }

  listen_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (listen_fd == -1) {
    string msg = "Error: cannot create socket\n";
    msg = msg + "  (" + hostname + "," + port + ")";
    throw myException(msg);
  }

  int yes = 1;
  status = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(listen_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    string msg = "Error: cannot bind socket\n";
    msg = msg + "  (" + hostname + "," + port + ")";
    throw myException(msg);
  }  // if

  status = listen(listen_fd, 100);
  if (status == -1) {
    string msg = "Error: cannot listen on socket\n";
    msg = msg + "  (" + hostname + "," + port + ")";
    throw myException(msg);
  }  // if

  freeaddrinfo(host_info_list);

  return listen_fd;
}
/**
 * accept connection on socket, using IPv4 only
 * @param ip
*/
void Server::acceptConnection(string & ip) {
  struct sockaddr_storage socket_addr;
  char str[INET_ADDRSTRLEN];
  socklen_t socket_addr_len = sizeof(socket_addr);
  client_connection_fd =
      accept(listen_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    string msg = "Error: cannot accept connection on socket";
    throw myException(msg);
  }  //if

  //only use IPv4
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  inet_ntop(socket_addr.ss_family,
            &(((struct sockaddr_in *)addr)->sin_addr),
            str,
            INET_ADDRSTRLEN);
  ip = str;
  writeLog("Connection accepted from client ip: " + ip);
}

/**
 * get the port number
 * @return port number
 */
int Server::getPort() {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(listen_fd, (struct sockaddr *)&sin, &len) == -1) {
    string msg = "Error: cannot get socket name";
    throw myException(msg);
  }
  return ntohs(sin.sin_port);
}

// write a fucntion to get the request from the client and parse it, and store to a outer char * buffer
void Server::getRequest(char * buffer) {
  int len_recv = recv(client_connection_fd, buffer, 4096, 0);
  cout << "len_recv: " << len_recv << endl;

  cout << "------" << endl;
  cout << "Request: " << buffer << endl;
  cout << "------" << endl;
}
/*
 * The CONNECT method requests that the recipient establish a tunnel to
 * the destination origin server identified by the request-target and,
 * if successful, thereafter restrict its behavior to blind forwarding
 * of packets, in both directions, until the tunnel is closed.
 * @param id client's unique id
 */
void Server::requestConnect(int id) {
  string msg = "HTTP/1.1 200 OK\r\n\r\n";
  int status = send(client_connection_fd, msg.c_str(), strlen(msg.c_str()), 0);
  if (status == -1) {
    string msg = "Error(Connection): message buffer being sent is broken";
    throw myException(msg);
  }
  writeLog(id + ": Responding \"HTTP/1.1 200 OK\"");
  fd_set read_fds;
  int maxfd = listen_fd > client_connection_fd ? listen_fd : client_connection_fd;
  while (true) {
    FD_ZERO(&read_fds);
    FD_SET(listen_fd, &read_fds);
    FD_SET(client_connection_fd, &read_fds);
    status = select(maxfd + 1, &read_fds, NULL, NULL, NULL);
    if (status == -1) {
      string msg = "Error(Connection): select() failed";
      throw myException(msg);
    }
    if (FD_ISSET(client_connection_fd, &read_fds)) {  //add try/catch
      connect_Transferdata(client_connection_fd, listen_fd);
    }
    else {
      connect_Transferdata(listen_fd, client_connection_fd);
    }
  }
  writeLog(id + ": Tunnel closed");
}

/*
 * Transfer Data between sender and receiver
 * @param send_fd
 * @param recv_fd
 */
void Server::connect_Transferdata(int send_fd, int recv_fd) {
  char buffer[65535] = {0};
  int len = send(send_fd, buffer, sizeof(buffer), 0);
  if (len <= 0) {
    throw myException("Error(CONNECT): transfer data failed.");
  }

  len = recv(recv_fd, buffer, sizeof(buffer), 0);
  if (len <= 0) {
    throw myException("Error(CONNECT): transfer data failed.");
  }
}