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
int Proxy::build_connection(const char * host, const char * port) {
  int status;
  int client_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(host, port, &host_info, &host_info_list);
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
 * get the port number of the connection
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

/**
 * The CONNECT method requests that the recipient establish a tunnel to
 * the destination origin server identified by the request-target and,
 * if successful, thereafter restrict its behavior to blind forwarding
 * of packets, in both directions, until the tunnel is closed.
 * @param client_fd client's socket fd
 * @param thread_id thread's id
 */
void Proxy::requestCONNECT(int client_fd, int thread_id) {
  std::string msg = "HTTP/1.1 200 OK\r\n\r\n";
  int status = send(client_connection_fd, msg.c_str(), strlen(msg.c_str()), 0);
  if (status == -1) {
    msg = "Error(Connection): message buffer being sent is broken";
    throw myException(msg);
  }
  msg = std::to_string(thread_id) + ": Responding \"HTTP/1.1 200 OK\"";
  logFile.log(msg);
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
      msg = "Error(Connection): select() failed";
      throw myException(msg);
    }
    if (FD_ISSET(client_connection_fd, &read_fds)) {
      if (!connect_Transferdata(client_connection_fd, client_fd)) {
        break;
      }
    }
    else if (FD_ISSET(client_fd, &read_fds)) {
      if (!connect_Transferdata(client_fd, client_connection_fd)) {
        break;
      }
    }
  }
  msg = std::to_string(thread_id) + ": Tunnel closed";
  logFile.log(msg);
  close(client_fd);
  close(client_connection_fd);
  return;
}

/**
 * Transfer Data between sender and receiver
 * @param fd1 send_fd
 * @param fd2 recv_fd
 */
bool Proxy::connect_Transferdata(int fd1, int fd2) {
  std::vector<char> buffer(MAX_LENGTH, 0);
  int total = 0;
  int n = 0;
  // if size is not enough
  while ((n = recv(fd1, &buffer[total], MAX_LENGTH, 0)) == MAX_LENGTH) {
    buffer.resize(total + 2 * MAX_LENGTH);
    total += n;
  }
  if (n < 0) {
    return false;
  }
  total += n;
  buffer.resize(total);

  if (buffer.empty()) {
    return false;
  }
  int status = send(fd2, &buffer.data()[0], buffer.size(), 0);
  if (status <= 0) {
    return false;
  }
  return true;
}

/**
 * send GET request to browser, check error case and send response to browser
 * @param client_fd 
 * @param request the request info sent by browser
 * @param thread_id 
 */
void Proxy::requestGET(int client_fd, httpcommand request, int thread_id) {
  std::string msg = std::to_string(thread_id) + ": Requesting \"" + request.method + " " +
                    request.url + "\" from " + request.host;
  logFile.log(msg);
  //  more secure way to send data
  const char * data = request.request.c_str();
  int request_len = request.request.size();
  int total_sent = 0;
  while (total_sent < request_len) {
    int sent = send(client_fd, data + total_sent, request_len - total_sent, 0);
    if (sent == -1) {
      msg = "Error(GET): message buffer being sent is broken";
      throw myException(msg);
    }
    total_sent += sent;
  }

  /*------- char [] way of receiving GET response from server --------*/
  char buffer[40960];
  memset(buffer, 0, sizeof(buffer));
  int recv_len = 0;
  ResponseInfo response_info;
  ssize_t recv_first = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
  if (recv_first == -1) {
    // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
    return;  // **** should be using return instead of exit ****
  }
  TimeMake t;
  std::string buffer_str(buffer);
  response_info.parseResponse(buffer_str, t.getTime());
  size_t pos = response_info.response.find_first_of("\r\n");
  std::string tolog = response_info.response.substr(0, pos);
  msg = std::to_string(thread_id) + ": Received \"" + tolog + "\" from " + request.host;
  logFile.log(msg);
  response_info.logCat(thread_id);  // print NOTE: ....
  if (response_info.is_chunk) {
    send(client_connection_fd, buffer, recv_first, 0);
    sendChunkPacket(client_fd, client_connection_fd);
  }
  else {
    // size_t header_end = buffer_str.find("\r\n\r\n");
    // int header_len = header_end + 4;
    // int recv_left = response_info.content_length - (recv_first - header_len);
    // int len = 0;
    // if (recv_left > 0) {
    //   recv_len = recv(client_fd, buffer + recv_first, sizeof(buffer) - recv_first, 0);
    // }
    // if (recv_len + recv_left < response_info.content_length) {
    //   response_info.isBadGateway = true;
    // }
    bool isBad = response_info.checkBadGateway(client_fd, thread_id);
    send(client_connection_fd, buffer, recv_first + recv_len, 0);
  }
  if (response_info.isCacheable(thread_id)) {
    cache.put(request.url, response_info);
  }
  std::string response_line = buffer_str.substr(0, buffer_str.find_first_of("\r\n"));
  msg = std::to_string(thread_id) + ": Responding \"" + response_line + "\"";
  logFile.log(msg);
  close(client_fd);
  close(client_connection_fd);
}

/**
 * Send chunk packets to browser, do not wait for the whole packet
 * @param client_fd  -- fd connected to server
 * @param client_connection_fd -- fd connected to browser
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
  std::string msg = std::to_string(thread_id) + ": Requesting \"" + request_info.method +
                    " " + request_info.url + "\" from " + request_info.host;
  logFile.log(msg);
  const char * data = request_info.request.c_str();
  int request_len = request_info.request.size();
  int total_sent = 0;
  while (total_sent < request_len) {
    int sent = send(client_fd, data + total_sent, request_len - total_sent, 0);
    if (sent == -1) {
      msg = "Error(GET): message buffer being sent is broken";
      throw myException(msg);
    }
    total_sent += sent;
  }
  // std::cout << data << std::endl;

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
  TimeMake t;

  response_info.parseResponse(buffer_str, t.getTime());
  size_t pos = response_info.response.find_first_of("\r\n");
  std::string tolog = response_info.response.substr(0, pos);
  msg = std::to_string(thread_id) + ": Received \"" + tolog + "\" from " +
        request_info.host;
  logFile.log(msg);
  int i = 0;
  while (recv_len < response_info.content_length) {
    ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
    if (n == -1) {
      // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
      return;  // **** should be using return instead of exit ****
    }
    if (n > 0 && i == 0) {
      msg = std::to_string(thread_id) + ": Responding \"" + tolog;
      logFile.log(msg);
    }
    recv_len += n;
  }

  // std::cout << "****************" << std::endl;
  // std::cout << buffer << std::endl;

  send(client_connection_fd, buffer, strlen(buffer), 0);
  close(client_fd);
  close(client_connection_fd);
}

void Proxy::handleRequest(int thread_id) {
  std::vector<char> buffer(MAX_LENGTH, 0);
  int idx = 0;
  int len_recv = recv(client_connection_fd, &(buffer.data()[idx]), MAX_LENGTH, 0);
  // std::cout << buffer.data() << std::endl;

  std::string client_request_str(buffer.data());
  httpcommand request_info(client_request_str);
  // std::cout << request_info.method << std::endl;

  if (!request_info.checkBadRequest(client_connection_fd, thread_id)) {
    close(client_connection_fd);
    return;
  }

  TimeMake currTime;
  std::string request_time = currTime.getTime();
  std::string msg = std::to_string(thread_id) + ": \"" + request_info.method + " " +
                    request_info.url + "\" from " + client_ip + " @ " + request_time;
  logFile.log(msg);

  try {
    // build connection with remote server
    int client_fd =
        build_connection(request_info.host.c_str(), request_info.port.c_str());
    if (request_info.method == "CONNECT") {
      msg = std::to_string(thread_id) + ": Requesting \"" + request_info.method + " " +
            request_info.url + "\" from " + request_info.host;
      logFile.log(msg);
      requestCONNECT(client_fd, thread_id);
      msg = std::to_string(thread_id) + ": Tunnel closed";
      logFile.log(msg);
    }
    else if (request_info.method == "GET") {
      if (!cache.has(request_info.url)) {  // not in cache
        msg = std::to_string(thread_id) + ": not in cache";
        logFile.log(msg);
        requestGET(client_fd, request_info, thread_id);
      }
      else {                                        // find in cache
        if (cache.get(request_info.url).noCache) {  // has no-cache
          // check validation
          if (!cache.validate(request_info.url, client_request_str)) {
            std::string req_msg_str = client_request_str;
            char req_new_msg[req_msg_str.size() + 1];
            send(client_fd, req_new_msg, req_msg_str.size() + 1, 0);
            char new_resp[65536] = {0};
            int new_len = recv(client_fd, &new_resp, sizeof(new_resp), 0);
            std::string checknew(new_resp, new_len);
            if (checknew.find("HTTP/1.1 200 OK") != std::string::npos) {
              msg = std::to_string(thread_id) + ": cached, but requires re-validation";
              logFile.log(msg);
            }
            requestGET(client_fd, request_info, thread_id);
          }
          else {  // use cache
            msg = std::to_string(thread_id) + ": in cache, valid";
            logFile.log(msg);
            cache.useCache(request_info, client_fd, thread_id);
          }
        }
        else {
          // check fresh
          std::string response_time = currTime.getTime();
          if (cache.get(request_info.url)
                  .isFresh(response_time, request_info.maxStale)) {  // use cache
            msg = std::to_string(thread_id) + ": in cache, valid";
            logFile.log(msg);
            cache.useCache(request_info, client_fd, thread_id);
          }
          else {
            msg = std::to_string(thread_id) + ": cached, expires at " +
                  currTime.getTime(cache.get(request_info.url).freshLifeTime);
            logFile.log(msg);
            requestGET(client_fd, request_info, thread_id);
          }
          cache.printCache();
        }
      }
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

/**
 * Handle requests from browser in multi-thread way
 * @param thread_id -- the thread id of a thread
 */
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