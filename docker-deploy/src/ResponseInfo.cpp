#include "ResponseInfo.h"

void ResponseInfo::setContentLength(string& buffer) {
    content_length = -1;
    std::size_t end = buffer.find("\r\n\r\n");
    if (end != std::string::npos) {
        std::string headers = buffer.substr(0, end);
        // use Boost to find if the header contains "Content-Length: "
        boost::regex re("Content-Length:\\s*(\\d+)");
        boost::smatch what;
        if (boost::regex_search(headers, what, re)) {
            // get the string after "Content-Length: "
            content_length = std::stoi(what[1].str());
        }
    }
}

void ResponseInfo::setChunked(string& buffer) {
    std::size_t pos = buffer.find("\r\n\r\n");
    if (pos != std::string::npos) {
        std::string headers = buffer.substr(0, pos);
        // use Boost to find if the header contains "Transfer-Encoding: chunked"
        boost::regex re("Transfer-Encoding: chunked");
        boost::smatch what;
        if (boost::regex_search(headers, what, re)) {
            is_chunk = true;
        } else {
            is_chunk = false;
        }
    }
}

void ResponseInfo::setStatusCode(string& buffer) {
    size_t status_start = buffer.find("HTTP/1.");
    if (status_start != string::npos) {
        status_code = stoi(buffer.substr(status_start + 9, 3));
    }
}

void ResponseInfo::checkStatus(string& buffer) {
    if (status_code == 200) {
        // do nothing
    } else if (status_code == 301) {
        // move permanently
        size_t location_start = buffer.find("Location:");
        if (location_start != string::npos) {
            size_t location_end = buffer.find("\r\n", location_start);
            string new_location = buffer.substr(location_start + 10, location_end - location_start - 10);
            // send new GET request to new location
            re_direct_url = new_location;
        }
    } else if (status_code == 304) {
        // do nothing
    } else if (status_code == 403) {
        // do nothing
    } else if (status_code == 404) {
        // do nothing
    } else if (status_code == 500) {
        // do nothing
    } else {
        // do nothing
    }
}

void ResponseInfo::setContentType(string& buffer) {
    size_t content_start = buffer.find("Content-Type:");
    if (content_start != string::npos) {
        size_t content_end = buffer.find(";", content_start);
        content_type = buffer.substr(content_start + 14, content_end - content_start - 14);
    }
}
