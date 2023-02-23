#include <string>
#include <vector>

//include header for string

using namespace std;


class RequestInfo {
   public:
    string request;  // GET / POST / CONNECT
    string method;
    string port;
    string host;
    string url;

    int contentLength;


    RequestInfo(){
    }

    int getContentLength(){
        //parse the string in the request to get the content length
        //return the content length


        return contentLength;
    }
};