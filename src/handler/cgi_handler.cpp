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
#include <vector>

void CgiHandler::setEnvVar(std::vector<std::string>& child_env_var)
{
    child_env_var.push_back("REQUEST_METHOD=" + saved_request_.method);
    child_env_var.push_back("SCRIPT_FILENAME=" + saved_request_.path);
    child_env_var.push_back("QUERY_STRING=" + saved_request_.query_string);
    child_env_var.push_back("CONTENT_LENGTH=" + itoa(saved_request_.content_length));
    // To do : NOT AVAILABLE IN HttpRequest. To be added if required by CgiHandler
    if (saved_request_.method == "POST") {
        child_env_var.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
    }
    child_env_var.push_back("SERVER_NAME=" + config_.server_name);
    // To do : NOT AVAILABLE IN CONFIG. To be added if required by CgiHandler
    child_env_var.push_back("SERVER_PROTOCOL=HTTP/1.1");
    child_env_var.push_back("SERVER_PORT=" + config_.listen_port);
}

// constructor needs to query the request, create the env var,
CgiHandler::CgiHandler(const std::string& path, const HttpRequest& saved_request,
                       const ServerConfig& config)
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
      input_fd{-1, -1},
      output_fd{-1, -1}
{
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

    if (pid_ == 0) {
        // child process - setting up pipes
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
        }
        // closing write end of the output_fd
        close(output_fd[1]);
    }
}

CgiHandler::~CgiHandler()
{
    if (input_fd[1] != -1)
        close(input_fd[1]);
    if (output_fd[0] != -1)
        close(output_fd[0]);
}

int CgiHandler::childReaped(void)
{
    if (child_reaped_) {
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

std::string build_error_response(HttpResponse::Status status, char* inline_body)
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
        trim(content_line);
        content_type = content_line;
    }

    // CGI spec: missing Content-Type → error
    if (content_type.empty()) {
        return 1;
    }

    // -------------------------------
    // PHASE 3 — Build real HTTP headers
    // -------------------------------
    res.code = status_code;
    res.content_type = content_type;
    return (0);
}

size_t CgiHandler::read_data(char* buf, size_t n)
{
    if (!headers_parsed_) {
        char tmp[4096];
        ssize_t bytes_read = read(output_fd[0], tmp, sizeof(tmp));
        if (bytes_read == -1) {
            headers_.append(
                build_error_response(HttpResponse::kStatusBadGateway, "<h1> 502 Bad Gateway <h1>"));
            headers_parsed_ =
                true; // enforcing that the headers are send on the next has_output call
            return;
        }
        raw_output_.append(tmp, bytes_read);
        if (bytes_read == 0 && raw_output_.find("\r\n\r\n") == std::string::npos) {
            headers_.append(
                build_error_response(HttpResponse::kStatusBadGateway, "<h1> 502 Bad Gateway <h1>"));
            headers_parsed_ =
                true; // enforcing that the headers are send on the next has_output call
            return;
        }
        size_t header_end = raw_output_.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return; // continue parsing
        }
        std::string cgi_headers = raw_output_.substr(0, header_end);
        std::string body_ = raw_output_.substr(header_end + 4);
        HttpResponse res;
        if (parse_headers(cgi_headers, res) != 0) {
            headers_.append(
                build_error_response(HttpResponse::kStatusBadGateway, "<h1> 502 Bad Gateway <h1>"));
            headers_parsed_ =
                true; // enforcing that the headers are send on the next has_output call
            return;
        }
        headers_.append(res.to_string());
        headers_parsed_ = true;
    }
    if (!headers_sent_) {
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

        return to_copy;
    }
    eoo_reached_ = true;
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
