#include "server.h"

#include "client.h"
#include "helper.h"
#include "httpcommand.h"
#include "myexception.h"

ofstream logFiles(logFileLocation);
void Server::writeLog(string msg) {
  lock_guard<mutex> lck(mtx);
  logFiles << msg << endl;
}

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
  }  // if

  // only use IPv4
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  inet_ntop(socket_addr.ss_family,
            &(((struct sockaddr_in *)addr)->sin_addr),
            str,
            INET_ADDRSTRLEN);
  ip = str;
  cout << "Connection accepted"
       << "from client ip: " << ip << endl;
  client_ip = ip;
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

/*
 * The CONNECT method requests that the recipient establish a tunnel to
 * the destination origin server identified by the request-target and,
 * if successful, thereafter restrict its behavior to blind forwarding
 * of packets, in both directions, until the tunnel is closed.
 * @param id client's unique id
 */
void Server::requestCONNECT(int client_fd, int thread_id) {
  string msg = "HTTP/1.1 200 OK\r\n\r\n";
  int status = send(client_connection_fd, msg.c_str(), strlen(msg.c_str()), 0);
  if (status == -1) {
    string msg = "Error(Connection): message buffer being sent is broken";
    throw myException(msg);
  }
  writeLog(thread_id + ": Responding \"HTTP/1.1 200 OK\"");
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
      string msg = "Error(Connection): select() failed";
      throw myException(msg);
    }
    if (FD_ISSET(client_connection_fd, &read_fds)) {
      try {
        connect_Transferdata(client_connection_fd, client_fd);
      }
      catch (myException & e) {
        cout << e.what() << endl;
        break;
      }
    }
    else {
      try {
        connect_Transferdata(client_fd, client_connection_fd);
      }
      catch (myException & e) {
        cout << e.what() << endl;
        break;
      }
    }
  }
  writeLog(thread_id + ": Tunnel closed");
  close(client_fd);
  close(client_connection_fd);
  return;
}

/*
 * Transfer Data between sender and receiver
 * @param send_fd
 * @param recv_fd
 */
void Server::connect_Transferdata(int fd1, int fd2) {
  vector<char> buffer(MAX_LENGTH, 0);
  int len = recv(fd1, &(buffer.data()[0]), MAX_LENGTH, 0);
  if (len <= 0) {
    throw myException("Error(CONNECT_recv): transfer data failed.");
  }
  len = send(fd2, &(buffer.data()[0]), MAX_LENGTH, 0);
  if (len <= 0) {
    throw myException("Error(CONNECT_send): transfer data failed.");
  }
}

void Server::handleRequest(int thread_id) {
  vector<char> buffer(4096);
  int endPos = 0;
  int byts = 0;
  int idx = 0;
  bool requestBody = false;
  while (true) {
    int len_recv = recv(client_connection_fd, &(buffer.data()[0]), 4096, 0);
    idx += len_recv;
    if (len_recv >= 0) {
      string client_request(buffer.data());
      //find header
      if ((endPos = client_request.find("\r\n\r\n")) != string::npos) {
        int contentPos = client_request.find("Content-Length:");
        if (contentPos == string::npos) {
          break;
        }
        string tmp = client_request.substr(contentPos);
        int numPos = tmp.find_first_of(':') + 2;
        int lbrPos = tmp.find_first_of('\r');
        if (!requestBody) {
          byts = std::stoi(tmp.substr(numPos, lbrPos - numPos));
          int currContBytes = idx - endPos - 4;
          if (currContBytes >= byts) {
            break;
          }
          else {
            byts = endPos + byts - idx + 4;
            byts += len_recv;
          }
          requestBody = true;
        }
        else {
          byts -= len_recv;
          if (byts <= 0) {  //body finish
            break;
          }
        }
      }
    }
    else {
      string msg = "Error(Handle):Failed to receive data";
      throw myException(msg);
    }
    if (idx == buffer.size()) {
      buffer.resize(2 * buffer.size());
    }
  }
  // int len_recv = recv(fd, &(buffer.data()[0]), 4096, 0);
  // if (len_recv == -1) {
  //   cout << "failed" << endl;
  //   return;
  // }
  string client_request(buffer.data());
  httpcommand h(client_request);

  if (client_request.find("Host:", 0) == string::npos) {
    string badRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";
    int status =
        send(client_connection_fd, badRequest.c_str(), strlen(badRequest.c_str()), 0);
    if (status < 0) {
      perror("send 400 to client");
    }
    close(client_connection_fd);
    return;
  }

  TimeMake currTime;
  writeLog(thread_id + ": \"" + h.request + "\" from " + client_ip + " @ " +
           currTime.getTime());
  Client client;
  try {
    client.initClientfd(h.host.c_str(), h.port.c_str());
  }
  catch (exception & e) {
    std::cout << e.what() << std::endl;
  }
  if (h.method == "CONNECT") {
    requestCONNECT(client.client_fd, thread_id);
  }
}

void Server::run(int thread_id) {
  handleRequest(thread_id);
}