#include "http/http_parser.hpp"

#include "http/http_version.hpp"
#include "util/log_message.hpp"
#include "util/string.hpp"

#include <cstdlib>
#include <cstring>

HttpParser::HttpParser()
    : state_(kParsingRequestLine),
      body_bytes_parsed_(0),
      body_bytes_read_(0)
{
}

void HttpParser::append_data(const char* src, size_t n)
{
    if (state_ == kParsingError || state_ == kParsingDone) {
        LOG(WARN) << "Parser rejected " << n << "bytes (invalid state)";
        return;
    }

    raw_buffer_.append(src, n);

    parse_request_line();
    parse_headers();
    parse_body();
}

void HttpParser::reset_for_next_request()
{
    assert(body_buffer_.empty());

    if (!raw_buffer_.empty())
        LOG(WARN) << "Server dropped " << raw_buffer_.size() << " bytes (pipelining not supported)";

    state_ = kParsingRequestLine;
    raw_buffer_.clear();
    body_buffer_.clear();
    body_bytes_parsed_ = 0;
    body_bytes_read_ = 0;
    req_ = HttpRequest();
}

size_t HttpParser::read_body_chunk(char* dest, size_t n)
{
    if (state_ != kParsingBody && state_ != kParsingDone)
        return 0;

    assert(body_bytes_read_ < req_.content_length);

    size_t to_copy = std::min(body_buffer_.size(), n);

    std::memcpy(dest, body_buffer_.data(), to_copy);
    body_bytes_read_ += to_copy;
    body_buffer_.erase(0, to_copy);

    return to_copy;
}

static bool is_valid_request_line(const std::vector<std::string>& v)
{
    if (v.size() != 3)
        return false;

    if (v[0] != "GET" && v[0] != "POST" && v[0] != "DELETE")
        return false;

    if (v[1].empty() || v[1][0] != '/')
        return false;

    if (http_version_from_string(v[2]) == kHttpVersionUnknown)
        return false;

    return true;
}

// Format: METHOD SP REQUEST-URI SP HTTP-VERSION CRLF
void HttpParser::parse_request_line()
{
    if (state_ != kParsingRequestLine)
        return;

    size_t crlf = raw_buffer_.find("\r\n");
    if (crlf == std::string::npos)
        return;

    std::string request_line = raw_buffer_.substr(0, crlf);
    std::vector<std::string> v = str_split(request_line, " ");

    if (!is_valid_request_line(v)) {
        state_ = kParsingError;
        return;
    }

    req_.method = v[0];
    req_.path = v[1];
    req_.http_version = http_version_from_string(v[2]);

    req_.keep_alive = (req_.http_version == kHttpVersion1_1) ? true : false;

    raw_buffer_.erase(0, crlf + 2);
    state_ = kParsingHeaders;
}

// Need to parse: content_length, is_chunked?, keep_alive?
void HttpParser::parse_headers()
{
    if (state_ != kParsingHeaders)
        return;

    size_t end = raw_buffer_.find("\r\n\r\n");

    if (end == std::string::npos)
        return;

    std::string header_block = raw_buffer_.substr(0, end);
    std::vector<std::string> v = str_split(header_block, "\r\n");

    for (size_t i = 0; i < v.size() && !v[i].empty(); i++) {

        size_t first_colon = v[i].find(':');
        if (first_colon == std::string::npos) {
            state_ = kParsingError;
            return;
        }

        std::string name = v[i].substr(0, first_colon);
        std::string value = str_trim(v[i].substr(first_colon + 1));

        if (name == "Content-Length") {
            // atoi is terrible but it will do the job for now
            req_.content_length = std::atoi(value.c_str());
        }
        else if (name == "Transfer-Encoding" && value == "chunked") {
            LOG(WARN) << "Chunked transfer not implemented yet!";
            req_.is_chunked = true;
        }
        else if (name == "Connection") {
            if (value == "keep-alive") {
                req_.keep_alive = true;
            }
            else if (value == "close") {
                req_.keep_alive = false;
            }
        }
        else {
            // Should probably ignore all these useless headers
            req_.headers[name] = value;
        }
    }
    raw_buffer_.erase(0, end + 4);
    state_ = kParsingBody;
}

void HttpParser::parse_body()
{
    if (state_ != kParsingBody)
        return;

    // No body to parse
    if (req_.content_length == 0) {
        state_ = kParsingDone;
        return;
    }

    assert(body_bytes_parsed_ < req_.content_length);

    size_t bytes_left = req_.content_length - body_bytes_parsed_;
    size_t to_copy = std::min(bytes_left, raw_buffer_.size());

    body_buffer_.append(raw_buffer_.data(), to_copy);
    raw_buffer_.erase(0, to_copy);

    body_bytes_parsed_ += to_copy;

    if (body_bytes_parsed_ == req_.content_length)
        state_ = kParsingDone;
}
