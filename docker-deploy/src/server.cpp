#include "server.h"
/*
  * init socket status
  * @param _hostname 
  * @param _port
*/
void Server::initStatus(const char * _hostname, const char * _port) {
  hostname = _hostname;
  port = _port;
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    exit(EXIT_FAILURE);
  }
}

/*
  * create socket, wait and stay listening
*/
void Server::createSocket() {
  if (port == NULL) {
    //OS will assign a port
    struct sockaddr_in * addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
    addr_in->sin_port = 0;
  }
  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    exit(EXIT_FAILURE);
  }

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    exit(EXIT_FAILURE);
  }  //if

  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    exit(EXIT_FAILURE);
  }  //if
  freeaddrinfo(host_info_list);
}

/*
  * accept connection on socket, using IPv4 only
  * @param ip
  * @return client_connection_fd
*/
int Server::acceptConnection(string & ip) {
  struct sockaddr_storage socket_addr;
  char str[INET_ADDRSTRLEN];
  socklen_t socket_addr_len = sizeof(socket_addr);
  client_connection_fd =
      accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return -1;
  }  //if

  //only use IPv4
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  inet_ntop(socket_addr.ss_family,
            &(((struct sockaddr_in *)addr)->sin_addr),
            str,
            INET_ADDRSTRLEN);
  ip = str;

  return client_connection_fd;
}

/*
  * get the port number
  * @return port number
*/
int Server::getPort() {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1) {
    cerr << "Error: cannot get socket name" << endl;
    exit(EXIT_FAILURE);
  }
  return ntohs(sin.sin_port);
}

/*
  * The CONNECT method requests that the recipient establish a tunnel to
  * the destination origin server identified by the request-target and,
  * if successful, thereafter restrict its behavior to blind forwarding
  * of packets, in both directions, until the tunnel is closed. 
  * @param thread_id
*/
void Server::requestConnect(int id) {
  string msg = "HTTP/1.1 200 OK\r\n\r\n";
  int status = send(client_connection_fd, msg.c_str(), strlen(msg.c_str()), 0);
  if (status == -1) {
    cerr << "Error(Connection): message buffer being sent is broken" << endl;
    return;
  }
  log << id << ": Responding \"HTTP/1.1 200 OK\"" << endl;
  fd_set read_fds;
  int maxfd = socket_fd > client_connection_fd ? socket_fd : client_connection_fd;
  while (true) {
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    FD_SET(client_connection_fd, &read_fds);
    status = select(maxfd + 1, &read_fds, NULL, NULL, NULL);
    if (status == -1) {
      cerr << "Error(Connection): select() failed" << endl;
      break;
    }

    connect_Transferdata(fd);
  }
}

/*
  * Transfer Data between sender and receiver
*/
void Server::connect_Transferdata(int fd[]) {
  int fd[2] = {socket_fd, client_connection_fd};
}