#include "server.h"
#include "httpcommand.h"


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
  * accept connection on socket
  * @param ip
  * @return client_connection_fd
*/
int Server::acceptConnection(string & ip) {
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  client_connection_fd =
      accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return -1;
  }  //if
  
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  ip = inet_ntoa(addr->sin_addr);

  cout << "Connection accepted" << "from client ip: " << ip << endl;
  return client_connection_fd;
}

/**
 * init socket and connect as a client
 * @param host
 * @param port
*/ 
int clientInit(const char *host, const char *port) {
    struct addrinfo client_info;
    struct addrinfo *client_info_list;
    int status;
    int socket_fd;

    memset(&client_info, 0, sizeof(client_info));
    client_info.ai_family = AF_UNSPEC;
    client_info.ai_socktype = SOCK_STREAM;

    // initialize the client_info_list, which is a linked list of addrinfo structs
    status = getaddrinfo(host, port, &client_info, &client_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host: " << host << endl;
        cerr << gai_strerror(status) << endl;
        return -1;
    }

    // socket_fd is the file descriptor for the socket that will listen for connections
    socket_fd = socket(client_info_list->ai_family,
                       client_info_list->ai_socktype,
                       client_info_list->ai_protocol);
    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << host << "," << port << ")" << endl;
        return -1;
    }

    // connect to the server
    status = connect(socket_fd, client_info_list->ai_addr, client_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << host << "," << port << ")" << endl;
        return -1;
    }
    cout << "Connected to " << host << " on port " << port << endl;

    // int my_id;
    // recv(socket_fd, &my_id, sizeof(my_id), 0);

    freeaddrinfo(client_info_list);
    // cout << "finished clientInit" << endl;
    return socket_fd;
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

// first creates and runs the server and listen for connections
void Server::run() {
  initStatus(NULL, "12345");
  createSocket();
  cout << "Server is listening on port: " << getPort() << endl;
  string ip;
  while (true) {
    client_connection_fd = acceptConnection(ip);
    if (client_connection_fd == -1) {
      continue;
    }
    char client_request[1024];
    getRequest(client_request);
    // converrt the client_request to string
    string client_request_str(client_request);
    httpcommand h(client_request_str);

    // print out the h object
    //cout << h << endl;
    cout << "------" << endl;
    cout << "Request: " << client_request << endl;
    cout << "------" << endl;
    //print size of httpcommand
    cout << "size of httpcommand: " << sizeof(h) << endl; 
    cout << "Method: " << h.method << endl;
    cout << "Path: " << h.url << endl;
    cout << "port: " << h.port << endl;
    
    cout << "------------------" << endl;
    cout << "init connection with web server" << endl; 
    int toweb_fd = clientInit(h.host.c_str(), h.port.c_str());
    send(toweb_fd, client_request, strlen(client_request), 0);
    char buffer[40960];
    cout << "done sending to web server" << endl;
    int len_recv = recv(toweb_fd, buffer, 40960, 0);
      cout << "len_recv: " << len_recv << endl;
    cout << "using strlen" << strlen(buffer) << endl; // this is wrong 
    cout << sizeof(buffer) << endl;
    cout << client_request << endl;
    send(client_connection_fd, client_request, sizeof(client_request), 0);
 
    //cout << "Connection accepted" << "from client ip: " << ip << endl;
    // handle the connection
    // ...
  }
}



// write a fucntion to get the request from the client and parse it, and store to a outer char * buffer
void Server::getRequest(char * buffer) {
  int len_recv = recv(client_connection_fd, buffer, 4096, 0);
  cout << "len_recv: " << len_recv << endl;
  // print out the h object
  //cout << h << endl;
  cout << "------" << endl;
  cout << "Request: " << buffer << endl;
  //cout << "Method: " << h.method << endl;
  //cout << "Path: " << h.url << endl;
  //cout << "port: " << h.port << endl;
}





