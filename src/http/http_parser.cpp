#include "http/http_parser.hpp"

#include "http/http_version.hpp"
#include "util/log_message.hpp"
#include "util/string.hpp"

#include <limits.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

HttpParser::HttpParser()
    : state_(HttpParser::kParsingRequestLine),
      current_chunk_size_(0),
      current_chunk_consumed_(0)
{
}

size_t HttpParser::available_chunk_size() const
{
    return std::min(recvbuf_.size(), current_chunk_size_ - current_chunk_consumed_);
}

bool HttpParser::has_body_chunk() const
{
    return available_chunk_size() > 0;
}

void HttpParser::append_data(const char* src, size_t n)
{
    assert(n <= available_size() && "recv buffer is full!");

    // NOTE: Would assert be better here?
    if (state_ == HttpParser::kParsingError || state_ == HttpParser::kParsingDone) {
        LOG(ERROR) << "Parser rejected " << n << "bytes (invalid state)";
        return;
    }

    recvbuf_.append(src, n);

    parse_request_line();
    parse_headers();
    parse_chunk_size();
    parse_chunk_crlf();
}

size_t HttpParser::read_next_body_chunk(char* dest, size_t n) const
{
    if (state_ != HttpParser::kParsingBody)
        return 0;

    size_t to_read = std::min(available_chunk_size(), n);

    std::memcpy(dest, recvbuf_.data(), to_read);

    return to_read;
}

void HttpParser::consume_body_chunk(size_t n)
{
    assert(n <= recvbuf_.size());
    assert(n <= available_chunk_size());

    recvbuf_.erase(0, n);
    current_chunk_consumed_ += n;

    if (current_chunk_consumed_ == current_chunk_size_) {
        if (req_.is_chunked) {
            state_ = HttpParser::kParsingChunkCrlf;
            parse_chunk_crlf();
            parse_chunk_size();
        }
        else {
            state_ = HttpParser::kParsingDone;
        }
    }
}

void HttpParser::reset_for_next_request()
{
    if (!recvbuf_.empty())
        LOG(WARN) << "Server dropped " << recvbuf_.size() << " bytes (pipelining not supported)";

    *this = HttpParser();
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
    // TODO: Add max request line size
    if (state_ != HttpParser::kParsingRequestLine)
        return;

    size_t crlf = recvbuf_.find("\r\n");
    if (crlf == std::string::npos)
        return;

    std::string request_line = recvbuf_.substr(0, crlf);
    std::vector<std::string> v = str_split(request_line, " ");

    if (!is_valid_request_line(v)) {
        state_ = HttpParser::kParsingError;
        return;
    }

    req_.method = v[0];

    size_t qpos = v[1].find("?");
    if (qpos != std::string::npos) {
        req_.path = v[1].substr(0, qpos);
        req_.query_string = v[1].substr(qpos + 1);
    }
    else {
        req_.path = v[1];
        req_.query_string = "";
    }

    req_.http_version = http_version_from_string(v[2]);
    req_.keep_alive = (req_.http_version == kHttpVersion1_1) ? true : false;

    recvbuf_.erase(0, crlf + 2);
    state_ = HttpParser::kParsingHeaders;
}

// Need to parse: content_length, is_chunked?, keep_alive?
void HttpParser::parse_headers()
{
    if (state_ != HttpParser::kParsingHeaders)
        return;

    size_t end = recvbuf_.find("\r\n\r\n");
    if (end == std::string::npos)
        return;

    std::string header_block = recvbuf_.substr(0, end);
    std::vector<std::string> v = str_split(header_block, "\r\n");

    // TODO: Refactor this
    for (size_t i = 0; i < v.size() && !v[i].empty(); i++) {
        size_t first_colon = v[i].find(':');
        if (first_colon == std::string::npos) {
            state_ = HttpParser::kParsingError;
            return;
        }

        std::string name = v[i].substr(0, first_colon);
        std::string value = str_trim(v[i].substr(first_colon + 1));

        if (name == "Content-Length") {
            // NOTE: atoi is terrible but it will do the job for now
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

    recvbuf_.erase(0, end + 4);

    if (req_.is_chunked) {
        state_ = HttpParser::kParsingChunkSize;
        current_chunk_size_ = 0;
    }
    else if (req_.content_length == 0) {
        state_ = HttpParser::kParsingDone;
        current_chunk_size_ = 0;
    }
    else {
        state_ = HttpParser::kParsingBody;
        current_chunk_size_ = req_.content_length;
    }
}

// 4 CRLF
// "Wiki" CRLF
// 7 CRLF
// "Pedia i" CRLF
// B CRLF
// "n " CRLF "chunks." CRLF
// 0 CRLF
// CRLF

void HttpParser::parse_chunk_size()
{
    if (state_ != HttpParser::kParsingChunkSize)
        return;

    size_t crlf = recvbuf_.find("\r\n");
    if (crlf == std::string::npos)
        return;

    std::string chunk_header = recvbuf_.substr(0, crlf);

    const char* beg = chunk_header.c_str();
    char* end = NULL;
    errno = 0;

    unsigned int val = std::strtoul(beg, &end, 16);
    if (end == beg) {
        state_ = HttpParser::kParsingError;
        return;
    }
    if (errno == ERANGE || val > WEBSERV_MAX_CHUNK_SIZE) {
        state_ = HttpParser::kParsingError;
        return;
    }
    if (end != beg + crlf) {
        state_ = HttpParser::kParsingError;
        return;
    }

    recvbuf_.erase(0, crlf + 2);
    current_chunk_size_ = val;
    current_chunk_consumed_ = 0;

    if (current_chunk_size_ == 0) {
        state_ = HttpParser::kParsingChunkCrlf;
    }
    else {
        state_ = HttpParser::kParsingBody;
    }
}

void HttpParser::parse_chunk_crlf()
{
    if (state_ != HttpParser::kParsingChunkCrlf)
        return;

    if (recvbuf_.size() < 2)
        return;

    if (recvbuf_.compare(0, 2, "\r\n") != 0) {
        state_ = HttpParser::kParsingError;
        return;
    }

    recvbuf_.erase(0, 2);

    if (current_chunk_size_ == 0) {
        state_ = HttpParser::kParsingDone;
    }
    else {
        state_ = HttpParser::kParsingChunkSize;
    }
}
