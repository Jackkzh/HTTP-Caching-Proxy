#include "proxy.h"

std::mutex mtx;
ofstream logFile(logFileLocation);

void writeLog(string msg) {
    lock_guard<mutex> lck(mtx);
    logFile << msg << endl;
}

/**
 * init socket status, bind socket, listen on socket as a server(to accept connection from browser)
 * @param port
 */
void Proxy::initListenfd(const char *port) {
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

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
        struct sockaddr_in *addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
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

/**
 * build connection to server, as a client
 * @param host
 * @param port
 * @return socket_fd
 */
int Proxy::build_connection(const char *host, const char *port) {
    int status;
    int client_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(host, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        return -1;
    }
    client_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (client_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        return -1;
    }
    status = connect(client_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        close(client_fd);
        return -1;
    }
    return client_fd;
}

/**
 * accept connection on socket, using IPv4 only
 * @param ip
 */
void Proxy::acceptConnection(string &ip) {
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
    struct sockaddr_in *addr = (struct sockaddr_in *)&socket_addr;
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
 * get the port number of the connection
 * @return port number
 */
int Proxy::getPort() {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1) {
        string msg = "Error: cannot get socket name";
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
            } catch (myException &e) {
                cout << e.what() << endl;
                break;
            }
        } else if (FD_ISSET(client_fd, &read_fds)) {
            try {
                connect_Transferdata(client_fd, client_connection_fd);
            } catch (myException &e) {
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

/**
 * Transfer Data between sender and receiver
 * @param fd1 -- send_fd
 * @param fd2 -- recv_fd
 */
void Proxy::connect_Transferdata(int fd1, int fd2) {
    vector<char> buffer(MAX_LENGTH, 0);
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

/**
 * send GET request to browser, check error case and send response to browser
 * @param client_fd
 * @param h
 * @param thread_id
 */
void Proxy::requestGET(int client_fd, httpcommand h, int thread_id) {
    // send(client_fd, h.request.c_str(), h.request.length(), 0);

    /*------------------send request to server--------------------*/
    //  more secure way to send data
    const char *data = h.request.c_str();
    int request_len = h.request.size();
    int total_sent = 0;
    while (total_sent < request_len) {
        int sent = send(client_fd, data + total_sent, request_len - total_sent, 0);
        if (sent == -1) {
            string msg = "Error(GET): message buffer being sent is broken";
            throw myException(msg);
        }
        total_sent += sent;
    }
    
    // cout << "successfully sent" << endl;
    /*------------------send request to server--------------------*/

    /*-------vector<char> type of receiving --------*/
    /* problem: free(): invalid next size (normal), Aborted (core dumped) */
    // vector<char> buffer_received(MAX_LENGTH, 0);
    // int total_received = 0;
    // // if size is not enough
    // while (true) {
    //     int received = recv(client_fd, &(buffer_received.data()[total_received]), MAX_LENGTH, 0);
    //     if (received < 0) {
    //         break;
    //         // cout << "here" << endl;
    //         // throw myException("Error(GET): message buffer being received is broken");
    //     } else if (received == 0) {
    //         break;
    //     } else {
    //     }
    //     if (total_received == buffer_received.size()) {
    //         buffer_received.resize(2 * buffer_received.size());
    //     }
    //     total_received += received;
    // }
    // cout << "successfully received" << endl;
    // // check if the buffer received from server is empty
    // if (buffer_received.empty()) {
    //     return;
    // }
    // cout << "--------------------" << endl;
    // cout << "the size of page " << total_received << endl;
    // cout << "--------------------" << endl;
    /*-------vector<char> type of receiving --------*/

    /*------- char [] way of receiving GET response from server --------*/
    char buffer[40960];
    memset(buffer, 0, sizeof(buffer));
    int recv_len = 0;
    ResponseInfo response_info;
    // while (true) {
    ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
    if (n == -1) {
        // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
        return;  // **** should be using return instead of exit ****
    }
    //cout << buffer << endl;
    string buffer_str(buffer);
    response_info.parseResponse(buffer_str);
    cout << "****************" << endl;
    cout << response_info.content_type << endl;
    cout << "is it chunk? " << response_info.is_chunk << endl;

    if (response_info.is_chunk) {
        send(client_connection_fd, buffer, n, 0);
        // break;
    }
    // if (n == -1) {
    //     perror("recv");  // **** not necessary an error, it's due to server side connection closed ****
    //     // exit(EXIT_FAILURE);
    //     return;  // **** should be using return instead of exit ****
    // } else if (n == 0) {
    //     //break;
    // } else {
    //     recv_len += n;
    // }
    // }
    /*------- char [] way of receiving GET response from server --------*/

    if (response_info.is_chunk) {
        sendChunkPacket(client_fd, client_connection_fd);
    } else {
        send(client_connection_fd, buffer, n, 0);
    }
    // send(client_connection_fd, buffer, recv_len, 0);
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
        vector<char> buffer_received(MAX_LENGTH, 0);
        int total_received = 0;
        int received = recv(client_fd, &(buffer_received.data()[total_received]), MAX_LENGTH, 0);
        if (received <= 0) {
            break;
            // throw myException("Error(GET): message buffer being received is broken");
        }
        send(client_connection_fd, &(buffer_received.data()[total_received]), received, 0);
    }
    // while (true) {
    //     char respChars[MAX_LENGTH] = {0};
    //     cout << "relat" << endl;
    //     int size = recv(client_fd, respChars, MAX_LENGTH, 0);
    //     if (size <= 0) {
    //         break;
    //     }
    //     send(client_connection_fd, respChars, size, 0);
    // }
}

/**
 * Handle POST request
 * @param client_fd -- fd connected to server
 * @param request_info -- httpcommand object storing the request information
 * @param thread_id -- the thread id of a thread
 */
void Proxy::requestPOST(int client_fd, httpcommand request_info, int thread_id) {
    cout << "in requestPOST" << endl;
    cout << "---" << endl;

    const char *data = request_info.request.c_str();
    int request_len = request_info.request.size();
    int total_sent = 0;
    while (total_sent < request_len) {
        int sent = send(client_fd, data + total_sent, request_len - total_sent, 0);
        if (sent == -1) {
            string msg = "Error(GET): message buffer being sent is broken";
            throw myException(msg);
        }
        total_sent += sent;
    }
    cout << data << endl;

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
    string buffer_str(buffer);
    response_info.parseResponse(buffer_str);
    while (recv_len < response_info.content_length) {
        ssize_t n = recv(client_fd, buffer + recv_len, sizeof(buffer) - recv_len, 0);
        if (n == -1) {
            // perror("recv"); // **** not necessarily an error, it's due to server side connection closed ****
            return;  // **** should be using return instead of exit ****
        }
        recv_len += n;
    }

    cout << "****************" << endl;
    cout << buffer << endl;

    send(client_connection_fd, buffer, strlen(buffer), 0);
    close(client_fd);
    close(client_connection_fd);
}

/**
 * Receive the request from browser, parse request type and call corresponding handler function
 * @param thread_id -- the thread id of a thread
 */
void Proxy::handleRequest(int thread_id) {
    /* -------- update --------  */
    // when receiving the 1st reponse packet from server, dont need while loop->to improve spped
    vector<char> buffer(MAX_LENGTH, 0);
    int endPos = 0, byts = 0, idx = 0;
    bool body = true;
    int len_recv = recv(client_connection_fd, &(buffer.data()[idx]), MAX_LENGTH, 0);
    // cout << "****** in handle request *******" << endl;
    cout << buffer.data() << endl;
    // cout << "---------*************----------" << endl;

    // while (true) {
    //     int len_recv = recv(client_connection_fd, &(buffer.data()[idx]), MAX_LENGTH, 0);
    //     idx += len_recv;
    //     if (len_recv >= 0) {
    //         string client_request(buffer.data());  // buffer.data() returns a pointer to the first element in the vector
    //         // find header
    //         if (!messageBodyHandler(len_recv, client_request, idx, body)) {
    //             break;
    //         }
    //     } else {
    //         string msg = "Error(Handle):Failed to receive data";
    //         throw myException(msg);
    //     }
    //     if (idx == buffer.size()) {
    //         buffer.resize(2 * buffer.size());
    //     }
    // }

    string client_request_str(buffer.data());
    httpcommand request_info(client_request_str);
    cout << request_info.method << endl;

    if (!checkBadRequest(client_request_str, client_connection_fd)) {
        close(client_connection_fd);
        return;
    }

    // write log
    TimeMake currTime;
    writeLog(thread_id + ": \"" + request_info.request + "\" from " + client_ip + " @ " +
             currTime.getTime());

    // Client client();  // create a Client object to store socket for browser and server
    try {
        // client.initClientfd(h.host.c_str(), h.port.c_str());
        /* ------ build connection with remote server ------ */
        // cout << "before connect to server" << endl;
        int client_fd = build_connection(request_info.host.c_str(), request_info.port.c_str());
        cout << "panduan " << endl;
        if (request_info.method == "CONNECT") {
            //cout << "its connect" << endl;
            requestCONNECT(client_fd, thread_id);

        }
        else if (request_info.method == "GET") {
            //cout << "its get" << endl;
            requestGET(client_fd, request_info, thread_id);
        }
        else if (request_info.method == "POST") {
            //cout << "in postr" << endl;
            cout << client_request_str << endl;
            requestPOST(client_fd, request_info, thread_id);
            // handle POST request
        }
    } catch (exception &e) {
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
    } catch (exception &e) {
        std::cout << e.what() << std::endl;
        return;
    }
    return;
}