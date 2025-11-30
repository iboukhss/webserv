#include "http/http_parser.hpp"

#include "util/log_message.hpp"
#include "util/string.hpp"

#include <cstdlib>
#include <cstring>

HttpParser::HttpParser()
    : status_(HttpParser::kIncomplete)
{
}

void HttpParser::feed_data(const char* buf, size_t n)
{
    if (status_ == kError || status_ == kBodyDone) {
        LOG(WARN) << "Parser rejected some data (" << n << "bytes)";
        return;
    }

    buffer_.append(buf, n);

    parse_request_line();
    parse_headers();
    parse_body();
}

static size_t min3(size_t a, size_t b, size_t c)
{
    return std::min(std::min(a, b), c);
}

size_t HttpParser::slurp_data(char* buf, size_t n)
{
    if (status_ != kHeadersDone)
        return 0;

    size_t bytes_left = req_.content_length - bytes_slurped_;
    size_t to_copy = min3(buffer_.size(), bytes_left, n);

    std::memcpy(buf, buffer_.data(), to_copy);
    bytes_slurped_ += to_copy;
    buffer_.erase(0, to_copy);

    if (bytes_slurped_ == req_.content_length)
        status_ = kBodyDone;

    return to_copy;
}

static bool is_valid_request_line(const std::vector<std::string>& v)
{
    if (v.size() != 3)
        return false;

    if (v[0] != "GET" && v[0] != "POST" && v[0] != "DELETE")
        return false;

    if (v[2] != "HTTP/1.0" && v[2] != "HTTP/1.1")
        return false;

    return true;
}

// Format: METHOD SP REQUEST-URI SP HTTP-VERSION CRLF
void HttpParser::parse_request_line()
{
    if (status_ != kIncomplete)
        return;

    size_t crlf = buffer_.find("\r\n");
    if (crlf == std::string::npos)
        return;

    std::string line = buffer_.substr(0, crlf);
    std::vector<std::string> v = str_split(line, " ");

    if (!is_valid_request_line(v)) {
        status_ = kError;
        return;
    }

    req_.method = v[0];
    req_.path = v[1];
    req_.http_version = v[2];

    buffer_.erase(0, crlf + 2);

    status_ = kRequestLineDone;
}

// Need to parse: content_length, is_chunked?, keep_alive?
void HttpParser::parse_headers()
{
    if (status_ != kRequestLineDone)
        return;

    size_t pos = buffer_.find("\r\n\r\n");
    if (pos == std::string::npos)
        return;

    std::vector<std::string> v = str_split(buffer_, "\r\n");

    for (size_t i = 0; i < v.size() && !v[i].empty(); i++) {

        size_t colon = v[i].find(':');
        if (colon == std::string::npos) {
            status_ = kError;
            return;
        }

        std::string name = v[i].substr(0, colon);
        std::string value = str_trim(v[i].substr(colon + 1));

        if (name == "Content-Length") {
            // This is terrible but it will do for now
            req_.content_length = std::atoi(value.c_str());
        }
        else if (name == "Transfer-Encoding" && value == "Chunked") {
            req_.is_chunked = true;
        }
        else if (name == "Connection" && value == "Keep-Alive") {
            req_.keep_alive = true;
        }
        else {
            req_.headers[name] = value;
        }
    }

    buffer_.erase(0, pos + 4);
    status_ = kHeadersDone;
}

void HttpParser::parse_body()
{
    if (status_ != kHeadersDone)
        return;

    if (req_.content_length == 0)
        status_ = kBodyDone;
}
