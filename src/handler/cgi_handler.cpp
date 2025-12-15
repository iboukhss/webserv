#include "handler/cgi_handler.hpp"

#include "config/server_config.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>
#include <sstream>
#include <vector>

static HttpResponse::Status status_from_int(int code)
{
    switch (code) {
    case 200: return HttpResponse::kStatusOk;
    case 201: return HttpResponse::kStatusCreated;
    case 204: return HttpResponse::kStatusNoContent;
    case 400: return HttpResponse::kStatusBadRequest;
    case 403: return HttpResponse::kStatusForbidden;
    case 404: return HttpResponse::kStatusNotFound;
    case 405: return HttpResponse::kStatusMethodNotAllowed;
    case 409: return HttpResponse::kStatusConflict;
    case 500: return HttpResponse::kStatusInternalServerError;
    case 501: return HttpResponse::kStatusNotImplemented;
    case 502: return HttpResponse::kStatusBadGateway;
    case 507: return HttpResponse::kStatusDiskFull;
    default:  return HttpResponse::kStatusInternalServerError;
    }
}

static std::string to_string_size_t(size_t v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

void CgiHandler::setEnvVar(std::vector<std::string>& child_env_var)
{
    child_env_var.push_back("REQUEST_METHOD=" + saved_request_.method);
    child_env_var.push_back("SCRIPT_FILENAME=" + saved_request_.path);
    child_env_var.push_back("QUERY_STRING=" + saved_request_.query_string);

    child_env_var.push_back("CONTENT_LENGTH=" + to_string_size_t(saved_request_.content_length));
    if (saved_request_.method == "POST") {
        child_env_var.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
    }
    child_env_var.push_back("SERVER_NAME=localhost"); // to be updated based on config or removed
    child_env_var.push_back("SERVER_PROTOCOL=HTTP/1.1");
    child_env_var.push_back("SERVER_PORT=" + to_string_size_t(WEBSERV_DEFAULT_PORT));
}

// constructor needs to query the request, create the env var,
CgiHandler::CgiHandler(const std::string& path, const HttpRequest& saved_request,
                       const RouteConfig& config)
    : path_(path),
      saved_request_(saved_request),
      config_(config),
      headers_off_(0),
      output_body_off_(0),
      bytes_written_body_(0),
      body_length_(saved_request.content_length),
      headers_parsed_(false),
      headers_sent_(false),
      eoo_reached_(false),
      child_reaped_(false),
      pipe_blocked_(false)
{
    input_fd[0] = -1;
    input_fd[1] = -1;
    output_fd[0] = -1;
    output_fd[1] = -1;

    // STEP 0 - Check that file exisst and is executable
    struct stat sb;
    if (stat(path.data(), &sb) == -1 || !S_ISREG(sb.st_mode) ||
        !(sb.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) || access(path.data(), X_OK) != 0) {
        // error
    }

    // STEP 1 - Prepare key=value pair vector to be passed to child process to set env var
    std::vector<std::string> child_env_var;
    std::vector<char*> envp;
    setEnvVar(child_env_var); // -> will be passed to execve( , , char *envp[])

    // STEP 2 - set up pipes
    if (saved_request_.method == "POST") {
        if (pipe(input_fd) == -1) {
            // some error to be sent
            return;
        }
    }

    if (pipe(output_fd) == -1) {
        // some error to be sent
        return;
    }

    // STEP 3 - Fork child process
    pid_ = fork();
    if (pid_ == -1) {
        // write some http error response
        return;
    }

    if (pid_ == 0) { // child process - setting up pipes
        // closing the write end and replacing STDIN by the read end of the input pipe
        if (saved_request_.method == "POST") {
            close(input_fd[1]);
            dup2(input_fd[0], STDIN_FILENO);
            close(input_fd[0]); // as now duplicated to STDIN
        }
        // closing the read end and replacing STDOUT by the write end of the output pipe
        close(output_fd[0]);
        dup2(output_fd[1], STDOUT_FILENO);
        close(output_fd[1]); // as now duplicated to STDOUT

        std::vector<char*> envp;
        envp.reserve(child_env_var.size() + 1);

        for (size_t i = 0; i < child_env_var.size(); ++i)
            envp.push_back(const_cast<char*>(child_env_var[i].c_str()));

        envp.push_back(NULL);

        char* argv[2];
        argv[0] = const_cast<char*>(saved_request_.path.c_str());
        argv[1] = NULL;

        execve(argv[0], argv, envp.data());
    }
    else {
        // parent process - setting up pipes
        // closing read end of input_fd
        if (saved_request_.method == "POST") {
            close(input_fd[0]);
            fcntl(input_fd[1], F_SETFL, O_NONBLOCK);
        }
        // closing write end of the output_fd
        close(output_fd[1]);
        fcntl(output_fd[0], F_SETFL, O_NONBLOCK);
    }
}

CgiHandler::~CgiHandler()
{
    if (input_fd[1] != -1) {
        close(input_fd[1]);
        input_fd[1] = -1;
    }

    if (output_fd[0] != -1) {
        close(output_fd[0]);
        output_fd[0] = -1;
    }
}

int CgiHandler::childReaped(void)
{
    if (child_reaped_) {
        // LOG(DEBUG) << "child_reaped == true";
        return 1;
    }

    int status;
    pid_t r = waitpid(pid_, &status, WNOHANG);
    if (r == 0) {
        return 0;
    }
    if (r == pid_) {
        child_reaped_ = true;
        return 1;
    }
    if (r == -1) {
        // error
        child_reaped_ = true;
        return 1;
    }
    return 0;
}

std::string build_error_response(HttpResponse::Status status, std::string inline_body)
{
    HttpResponse res;
    res.code = status;
    res.content_type = "text/html; charset=UTF-8";
    res.inline_body = inline_body;
    std::string headers_ = res.to_string();
    return (headers_);
}

int CgiHandler::parse_headers(std::string& cgi_headers, HttpResponse& res)
{
    // Extract status code
    int status_code = 200; // = default
    size_t pos_status = cgi_headers.find("Status:");
    if (pos_status != std::string::npos) {
        size_t line_end = cgi_headers.find("\n", pos_status);
        std::string st_line = cgi_headers.substr(pos_status, line_end - pos_status);
        int code = atoi(st_line.c_str() + 7);
        if (code > 0)
            status_code = code;
    }

    // Extract Content-Type
    std::string content_type = "";
    size_t pos_content = cgi_headers.find("Content-Type:");
    if (pos_content != std::string::npos) {
        size_t line_end = cgi_headers.find("\n", pos_content);
        std::string content_line =
            cgi_headers.substr(pos_content + 13, line_end - (pos_content + 13));
        // trim(content_line);
        content_type = content_line;
    }

    // CGI spec: missing Content-Type → error
    if (content_type.empty()) {
        return 1;
    }
    res.code = status_from_int(status_code);
    res.content_type = content_type;
    LOG(DEBUG) << res.to_string();
    return (0);
}

bool CgiHandler::has_output() const
{
    // Have something already buffered for socket
    if (headers_parsed_ && !headers_sent_)
        return true;
    if (output_body_off_ < output_body_.size())
        return true;

    // If CGI finished (EOF) we may be done (even if empty body)
    if (eoo_reached_)
        return true; // allows is_done() to progress cleanly

    // Otherwise: nothing buffered, not EOF => DO NOT claim output
    return false;
}

size_t CgiHandler::read_data(char* buf, size_t n)
{
    LOG(DEBUG) << "read_data()";
    childReaped();

    if (headers_parsed_ && !headers_sent_) {
        size_t remain = headers_.size() - headers_off_;
        size_t to_copy = std::min(remain, n);

        memcpy(buf, headers_.data() + headers_off_, to_copy);
        headers_off_ += to_copy;

        if (headers_off_ == headers_.size())
            headers_sent_ = true;

        return to_copy;
    }
    if (output_body_off_ < output_body_.size()) {
        size_t remain = output_body_.size() - output_body_off_;
        size_t to_copy = std::min(remain, n);

        memcpy(buf, output_body_.data() + output_body_off_, to_copy);
        output_body_off_ += to_copy;

        if (output_body_off_ == output_body_.size()) {
            output_body_.clear();
            output_body_off_ = 0;
        }

        return to_copy;
    }

    char tmp[4096];

    ssize_t bytes_read = read(output_fd[0], tmp, sizeof(tmp));
    LOG(DEBUG) << "bytes_read = " << bytes_read;
    if (bytes_read == 0) {
        eoo_reached_ = true;
        close(output_fd[0]);
        output_fd[0] = -1;
        return 0; // finished reading
    }
    if (bytes_read < 0) {
        headers_.append(
            build_error_response(HttpResponse::kStatusBadGateway, "<h1> 502 Bad Gateway 1 <h1>"));

        headers_parsed_ = true;
        headers_sent_ = false;
        headers_off_ = 0;
        eoo_reached_ = true;
        pipe_blocked_ = false;
        return 0;
    }

    if (!headers_parsed_) {
        raw_output_.append(tmp, bytes_read);

        size_t header_end = raw_output_.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            // Still waiting for full CGI header block
            return 0;
        }

        std::string cgi_headers = raw_output_.substr(0, header_end);
        std::string remainder = raw_output_.substr(header_end + 4);

        HttpResponse res;
        if (parse_headers(cgi_headers, res) != 0) {
            headers_.append(build_error_response(HttpResponse::kStatusBadGateway,
                                                 "<h1>502 Bad Gateway 2</h1>"));
        }
        else {
            // Build real HTTP headers (your to_string should include content-type, etc.)
            headers_.append(res.to_string());
        }

        headers_parsed_ = true;
        headers_sent_ = false;
        headers_off_ = 0;

        // Buffer any bytes that were already part of body
        output_body_.append(remainder);

        // Clear raw buffer (we don't need it anymore once headers are parsed)
        raw_output_.clear();
        return 0;
    }

    output_body_.append(tmp, bytes_read);
    return 0;
}

// We never write to this handler (read-only)
size_t CgiHandler::write_data(const char* buf, size_t n)
{
    ssize_t bytes = write(input_fd[1], buf, n);
    if (bytes == -1) {
        eob_reached_ = true;
        if (headers_.empty()) {
            HttpResponse res;
            res.code = HttpResponse::kStatusInternalServerError;
            res.inline_body =
                "<h1> Body could not be written to STDIN </h1>"; // to display a message
                                                                 // during testing
            headers_ = res.to_string();
            bytes_written_body_ = body_length_; // to ensure needs_input returns false
        }
        return 0;
    }
    bytes_written_body_ += bytes;
    if (bytes_written_body_ < body_length_) {

        return (bytes);
    }
    close(input_fd[1]);
    input_fd[1] = -1;
    LOG(INFO) << "CgiHandler : Body fully written to STDIN";
    return (0);
}
