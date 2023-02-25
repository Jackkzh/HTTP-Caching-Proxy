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
  msg = std::to_string(thread_id) + ": Tunnel closed";
  logFile.log(msg);
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
  std::string msg = std::to_string(thread_id) + ": Requesting \"" + h.method + " " +
                    h.url + "\" from " + h.host;
  logFile.log(msg);
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
  TimeMake t;
  std::string buffer_str(buffer);
  response_info.parseResponse(buffer_str, t.getTime());
  response_info.logCat(thread_id);
  if (response_info.is_chunk) {
    send(client_connection_fd, buffer, n, 0);
    sendChunkPacket(client_fd, client_connection_fd);
  }
  else {
    send(client_connection_fd, buffer, n, 0);
  }
  if (response_info.isCacheable(thread_id)) {
    cache.put(h.url, response_info);
  }
  std::string response_line = buffer_str.substr(0, buffer_str.find_first_of("\r\n"));
  msg = std::to_string(thread_id) + ": Responding \"" + response_line + "\"";
  logFile.log(msg);
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
  // std::cout << "in requestPOST" << std::endl;
  // std::cout << "---" << std::endl;
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
  TimeMake t;
  response_info.parseResponse(buffer_str, t.getTime());
  while (recv_len < response_info.content_length) {
    ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
    if (n == -1) {
      // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
      return;  // **** should be using return instead of exit ****
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
  std::string request_time = currTime.getTime();
  std::string msg = std::to_string(thread_id) + ": \"" + request_info.method + " " +
                    request_info.url + "\" from " + client_ip + " @ " + request_time;
  logFile.log(msg);

  try {
    // client.initClientfd(h.host.c_str(), h.port.c_str());

    // build connection with remote server
    int client_fd =
        build_connection(request_info.host.c_str(), request_info.port.c_str());
    if (request_info.method == "CONNECT") {
      msg = std::to_string(thread_id) + ": Requesting \"" + request_info.method + " " +
            request_info.url + "\" from " + request_info.host;
      logFile.log(msg);
      requestCONNECT(client_fd, thread_id);
    }
    else if (request_info.method == "GET") {
      if (!cache.has(request_info.url)) {  //not in cache
        msg = std::to_string(thread_id) + ": not in cache";
        logFile.log(msg);
        requestGET(client_fd, request_info, thread_id);
      }
      else {                                        //find in cache
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
          else {  //use cache
            char res[cache.get(request_info.url).response.size()];
            strcpy(res, cache.get(request_info.url).response.c_str());
            send(client_fd, res, cache.get(request_info.url).response.size(), 0);
            msg = std::to_string(thread_id) + ": Responding \"" + request_info.url + "\"";
            logFile.log(msg);
          }
        }
        else {
          // check fresh
          std::string response_time = currTime.getTime();
          if (cache.get(request_info.url).isFresh(response_time)) {  //use cache
            char res[cache.get(request_info.url).response.size()];
            strcpy(res, cache.get(request_info.url).response.c_str());
            send(client_fd, res, cache.get(request_info.url).response.size(), 0);
            msg = std::to_string(thread_id) + ": Responding \"" + request_info.url + "\"";
            logFile.log(msg);
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