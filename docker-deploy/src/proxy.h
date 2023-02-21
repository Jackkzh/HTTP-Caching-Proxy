#ifndef __PROXY__H__
#define __PROXY__H__
#include <mutex>

#include "client.h"
#include "helper.h"
#include "httpcommand.h"
#include "myexception.h"
std::mutex mtx;
ofstream logFile(logFileLocation);
void writeLog(string msg) {
  lock_guard<mutex> lck(mtx);
  logFile << msg << endl;
}
class Proxy {
  int socket_fd;
  int client_connection_fd;

 private:
  string client_ip;

 public:
  Proxy() {}
  Proxy(int socket_fd, int client_connection_fd) :
      socket_fd(socket_fd), client_connection_fd(client_connection_fd) {}

  /*
 * init socket status, bind socket, listen on socket as a server(to accept connection from browser)
 * @param port
 * @return listen_fd
 */
  void initListenfd(const char * port) {
    int status;
    struct addrinfo host_info;
    struct addrinfo * host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, port, &host_info, &host_info_list);

    if (status != 0) {
      string msg = "Error: cannot get address info for host\n";
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
      string msg = "Error: cannot create socket\n";
      msg = msg + "  (" + "" + "," + port + ")";
      throw myException(msg);
    }

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      string msg = "Error: cannot bind socket\n";
      msg = msg + "  (" + "" + "," + port + ")";
      throw myException(msg);
    }  // if

    status = listen(socket_fd, 100);
    if (status == -1) {
      string msg = "Error: cannot listen on socket\n";
      msg = msg + "  (" + "" + "," + port + ")";
      throw myException(msg);
    }  // if

    freeaddrinfo(host_info_list);
  }

  int build_connection(string host, string port) {
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo * host_info_list;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(host.c_str(), port.c_str(), &host_info, &host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      return -1;
    }
    socket_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      return -1;
    }
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      close(socket_fd);
      return -1;
    }
    return socket_fd;
  }

  /**
 * accept connection on socket, using IPv4 only
 * @param ip
 */
  void acceptConnection(string & ip) {
    struct sockaddr_storage socket_addr;
    char str[INET_ADDRSTRLEN];
    socklen_t socket_addr_len = sizeof(socket_addr);
    client_connection_fd =
        accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
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
    // cout << "Connection accepted"
    //      << "from client ip: " << ip << endl;
    client_ip = ip;
    // writeLog("Connection accepted from client ip: " + ip);
  }

  /**
 * get the port number
 * @return port number
 */
  int getPort() {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1) {
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
  void requestCONNECT(int client_fd, int thread_id) {
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
      else if (FD_ISSET(client_fd, &read_fds)) {
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
  void connect_Transferdata(int fd1, int fd2) {
    vector<char> buffer(MAX_LENGTH, 0);
    int total = 0;
    int n = 0;
    //if size is not enough
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

  void handleRequest(int thread_id) {
    vector<char> buffer(MAX_LENGTH, 0);
    int endPos = 0, byts = 0, idx = 0;
    bool body = true;
    while (true) {
      int len_recv = recv(client_connection_fd, &(buffer.data()[0]), MAX_LENGTH, 0);
      idx += len_recv;
      if (len_recv >= 0) {
        string client_request(buffer.data());
        //find header
        if (!messageBodyHandler(len_recv, client_request, idx, body)) {
          break;
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

    string client_request(buffer.data());
    httpcommand h(client_request);

    if (!checkBadRequest(client_request, client_connection_fd)) {
      close(client_connection_fd);
      return;
    }

    TimeMake currTime;
    writeLog(thread_id + ": \"" + h.request + "\" from " + client_ip + " @ " +
             currTime.getTime());

    Client client;
    try {
      int client_fd = build_connection(h.host.c_str(), h.port.c_str());
      if (h.method == "CONNECT") {
        requestCONNECT(client_fd, thread_id);
      }
      else if (h.method == "GET" || h.method == "POST") {
        requestGET(client_fd, h, thread_id);
      }
    }
    catch (exception & e) {
      std::cout << e.what() << std::endl;
      return;
    }
  }

  void requestGET(int client_fd, httpcommand h, int thread_id) {
    send(client_fd, h.request.c_str(), h.request.length(), 0);
    char buffer[40960];

    int recv_len = 0;
    while (true) {
      ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
      if (n == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
      }
      else if (n == 0) {
        break;
      }
      else {
        recv_len += n;
      }
    }
    send(client_connection_fd, buffer, sizeof(buffer), 0);
    close(client_fd);
    close(client_connection_fd);
  }

  void run(int thread_id) {
    try {
      handleRequest(thread_id);
    }
    catch (exception & e) {
      std::cout << e.what() << std::endl;
      return;
    }
    return;
  }
};
#endif