#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <string>
#include <vector>

using boost::asio::ip::tcp;

class HttpResponse {
 public:
  enum class Status {
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
  };

  HttpResponse() : status(Status::OK), content(""), headers() {}

  Status status;
  std::string content;
  std::vector<std::pair<std::string, std::string> > headers;

  void parse(boost::asio::streambuf & response) {
    std::istream response_stream(&response);

    // Parse status line
    std::string http_version, status_code, status_message;
    response_stream >> http_version >> status_code;
    std::getline(response_stream, status_message);
    status = static_cast<Status>(std::stoi(status_code));

    // Parse headers
    std::string header;
    while (std::getline(response_stream, header) && header != "\r") {
      std::vector<std::string> header_parts;
      boost::split(header_parts, header, boost::is_any_of(": "));
      if (header_parts.size() >= 2) {
        headers.emplace_back(header_parts[0], header_parts[1]);
      }
    }

    // Parse content
    std::ostringstream content_stream;
    content_stream << response_stream.rdbuf();
    content = content_stream.str();
  }
};
