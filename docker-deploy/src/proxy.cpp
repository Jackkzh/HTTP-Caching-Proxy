#include "proxy.h"

#include "ResponseInfo.h"
#include "cache.h"
#include "helper.h"
#include "httpcommand.h"
#include "myexception.h"

Logger logFile;
Cache cache(10 * 1024 * 1024);

/**
 * init socket status, bind socket, listen on socket as a server(to accept connection from browser)
 * @param port
 * @return listen_fd
 */
void Proxy::initListenfd(const char * port) {
  int status;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, port, &host_info, &host_info_list);

  if (status != 0) {
    std::string msg = "Error: cannot get address info for host\n";
    msg = msg + "  (" + "" + "," + port + ")";
    throw myException(msg);
  }

  if (port == "") {
    // OS will assign a port
    struct sockaddr_in * addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
    addr_in->sin_port = 0;
  }

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    std::string msg = "Error: cannot create socket\n";
    msg = msg + "  (" + "" + "," + port + ")";
    throw myException(msg);
  }

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::string msg = "Error: cannot bind socket\n";
    msg = msg + "  (" + "" + "," + port + ")";
    throw myException(msg);
  }  // if

  status = listen(socket_fd, 100);
  if (status == -1) {
    std::string msg = "Error: cannot listen on socket\n";
    msg = msg + "  (" + "" + "," + port + ")";
    throw myException(msg);
  }  // if

  freeaddrinfo(host_info_list);
}

/**
 * build connection to server, as a client
 * @param host
 * @param port
 * @return socket_fd
 */
int Proxy::build_connection(std::string host, std::string port) {
  int status;
  int client_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(host.c_str(), port.c_str(), &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    return -1;
  }
  client_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (client_fd == -1) {
    std::cerr << "Error: cannot create socket" << std::endl;
    return -1;
  }
  status = connect(client_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "Error: cannot connect to socket" << std::endl;
    close(client_fd);
    return -1;
  }
  return client_fd;
}

/**
 * accept connection on socket, using IPv4 only
 * @param ip
 */
void Proxy::acceptConnection(std::string & ip) {
  struct sockaddr_storage socket_addr;
  char str[INET_ADDRSTRLEN];
  socklen_t socket_addr_len = sizeof(socket_addr);
  client_connection_fd =
      accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    std::string msg = "Error: cannot accept connection on socket";
    throw myException(msg);
  }  // if

  // only use IPv4
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  inet_ntop(socket_addr.ss_family,
            &(((struct sockaddr_in *)addr)->sin_addr),
            str,
            INET_ADDRSTRLEN);
  ip = str;
  client_ip = ip;
}

/**
 * get the port number
 * @return port number
 */
int Proxy::getPort() {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1) {
    std::string msg = "Error: cannot get socket name";
    throw myException(msg);
  }
  return ntohs(sin.sin_port);
}

/*
 * The CONNECT method requests that the recipient establish a tunnel to
 * the destination origin server identified by the request-target and,
 * if successful, thereafter restrict its behavior to blind forwarding
 * of packets, in both directions, until the tunnel is closed.
 * @param id client's unique id
 */
void Proxy::requestCONNECT(int client_fd, int thread_id) {
  std::string msg = "HTTP/1.1 200 OK\r\n\r\n";
  int status = send(client_connection_fd, msg.c_str(), strlen(msg.c_str()), 0);
  if (status == -1) {
    std::string msg = "Error(Connection): message buffer being sent is broken";
    throw myException(msg);
  }
  logFile.log(thread_id + ": Responding \"HTTP/1.1 200 OK\"");
  fd_set read_fds;
  int maxfd = client_fd > client_connection_fd ? client_fd : client_connection_fd;
  while (true) {
    FD_ZERO(&read_fds);
    FD_SET(client_fd, &read_fds);
    FD_SET(client_connection_fd, &read_fds);

    struct timeval time;
    time.tv_sec = 0;
    time.tv_usec = 1000000000;

    status = select(maxfd + 1, &read_fds, NULL, NULL, &time);
    if (status == -1) {
      std::string msg = "Error(Connection): select() failed";
      throw myException(msg);
    }
    if (FD_ISSET(client_connection_fd, &read_fds)) {
      try {
        connect_Transferdata(client_connection_fd, client_fd);
      }
      catch (myException & e) {
        std::cout << e.what() << std::endl;
        break;
      }
    }
    else if (FD_ISSET(client_fd, &read_fds)) {
      try {
        connect_Transferdata(client_fd, client_connection_fd);
      }
      catch (myException & e) {
        std::cout << e.what() << std::endl;
        break;
      }
    }
  }
  logFile.log(thread_id + ": Tunnel closed");
  close(client_fd);
  close(client_connection_fd);
  return;
}

/*
 * Transfer Data between sender and receiver
 * @param send_fd
 * @param recv_fd
 */
void Proxy::connect_Transferdata(int fd1, int fd2) {
  std::vector<char> buffer(MAX_LENGTH, 0);
  int total = 0;
  int n = 0;
  // if size is not enough
  while ((n = recv(fd1, &buffer[total], MAX_LENGTH, 0)) == MAX_LENGTH) {
    buffer.resize(total + 2 * MAX_LENGTH);
    total += n;
  }
  total += n;
  buffer.resize(total);

  if (buffer.empty()) {
    return;
  }
  int status = send(fd2, &buffer.data()[0], buffer.size(), 0);
  if (status == -1) {
    throw myException("Error(CONNECT_send): transfer data failed.");
  }
}

void Proxy::requestGET(int client_fd, httpcommand h, int thread_id) {
  //  more secure way to send data
  const char * data = h.request.c_str();
  int request_len = h.request.size();
  int total_sent = 0;
  while (total_sent < request_len) {
    int sent = send(client_fd, data + total_sent, request_len - total_sent, 0);
    if (sent == -1) {
      std::string msg = "Error(GET): message buffer being sent is broken";
      throw myException(msg);
    }
    total_sent += sent;
  }

  /*------- char [] way of receiving GET response from server --------*/
  char buffer[40960];
  memset(buffer, 0, sizeof(buffer));
  int recv_len = 0;
  ResponseInfo response_info;
  ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
  if (n == -1) {
    // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
    return;  // **** should be using return instead of exit ****
  }

  std::string buffer_str(buffer);
  response_info.parseResponse(buffer_str);
  std::cout << "****************" << std::endl;
  std::cout << buffer_str << std::endl;
  response_info.printCacheFields();
  std::cout << "****************" << std::endl;
  std::cout << "is it chunk? " << response_info.is_chunk << std::endl;
  if (response_info.is_chunk) {
    send(client_connection_fd, buffer, n, 0);
    // break;
  }
  if (response_info.is_chunk) {
    sendChunkPacket(client_fd, client_connection_fd);
  }
  else {
    send(client_connection_fd, buffer, n, 0);
  }
  // send(client_connection_fd, buffer, recv_len, 0);
  close(client_fd);
  close(client_connection_fd);
}

/**
 *
 */
void Proxy::sendChunkPacket(int client_fd, int client_connection_fd) {
  while (true) {
    std::vector<char> buffer_received(MAX_LENGTH, 0);
    int total_received = 0;
    int received =
        recv(client_fd, &(buffer_received.data()[total_received]), MAX_LENGTH, 0);
    if (received <= 0) {
      break;
    }
    send(client_connection_fd, &(buffer_received.data()[total_received]), received, 0);
  }
}

/**
 * Handle POST request
 * @param client_fd -- fd connected to server
 * @param request_info -- httpcommand object storing the request information
 * @param thread_id -- the thread id of a thread
 */
void Proxy::requestPOST(int client_fd, httpcommand request_info, int thread_id) {
  std::cout << "in requestPOST" << std::endl;
  std::cout << "---" << std::endl;

  const char * data = request_info.request.c_str();
  int request_len = request_info.request.size();
  int total_sent = 0;
  while (total_sent < request_len) {
    int sent = send(client_fd, data + total_sent, request_len - total_sent, 0);
    if (sent == -1) {
      std::string msg = "Error(GET): message buffer being sent is broken";
      throw myException(msg);
    }
    total_sent += sent;
  }
  std::cout << data << std::endl;

  char buffer[40960];
  memset(buffer, 0, sizeof(buffer));
  int recv_len = 0;
  ResponseInfo response_info;

  ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
  if (n == -1) {
    // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
    return;  // **** should be using return instead of exit ****
  }
  recv_len += n;
  std::string buffer_str(buffer);
  response_info.parseResponse(buffer_str);
  while (recv_len < response_info.content_length) {
    ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
    if (n == -1) {
      // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
      return;  // **** should be using return instead of exit ****
    }
    recv_len += n;
  }

  std::cout << "****************" << std::endl;
  std::cout << buffer << std::endl;

  send(client_connection_fd, buffer, strlen(buffer), 0);
  close(client_fd);
  close(client_connection_fd);
}

void Proxy::handleRequest(int thread_id) {
  std::vector<char> buffer(MAX_LENGTH, 0);
  int endPos = 0, byts = 0, idx = 0;
  bool body = true;
  int len_recv = recv(client_connection_fd, &(buffer.data()[idx]), MAX_LENGTH, 0);
  std::cout << buffer.data() << std::endl;

  std::string client_request_str(buffer.data());
  httpcommand request_info(client_request_str);
  std::cout << request_info.method << std::endl;

  if (!checkBadRequest(client_request_str, client_connection_fd)) {
    close(client_connection_fd);
    return;
  }

  TimeMake currTime;
  logFile.log(thread_id + ": \"" + request_info.method + " " + request_info.url +
              "\" from " + client_ip + " @ " + currTime.getTime());

  try {
    // client.initClientfd(h.host.c_str(), h.port.c_str());

    // build connection with remote server
    int client_fd =
        build_connection(request_info.host.c_str(), request_info.port.c_str());
    if (request_info.method == "CONNECT") {
      requestCONNECT(client_fd, thread_id);
    }
    else if (request_info.method == "GET") {
      requestGET(client_fd, request_info, thread_id);
    }
    else if (request_info.method == "POST") {
      requestPOST(client_fd, request_info, thread_id);
    }
  }
  catch (std::exception & e) {
    std::cout << e.what() << std::endl;
    return;
  }
}

void Proxy::run(int thread_id) {
  try {
    handleRequest(thread_id);
  }
  catch (std::exception & e) {
    std::cout << e.what() << std::endl;
    return;
  }
  return;
}