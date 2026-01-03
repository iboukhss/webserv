#include "handler/cgi_handler.hpp"

#include "config/server_config.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/string.hpp"
#include "util/syscall_error.hpp"
#include "util/to_string.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>
#include <vector>

static std::vector<char*> make_envp(std::vector<std::string>& env);

void CgiHandler::init_state()
{
    input_fd_[0] = -1;
    input_fd_[1] = -1;
    output_fd_[0] = -1;
    output_fd_[1] = -1;
}

bool CgiHandler::validate_cgi_target(bool is_interpreter_cgi)
{
    struct stat sb;

    // Script must exist and be a regular file
    if (stat(path_.c_str(), &sb) == -1 || !S_ISREG(sb.st_mode)) {
        set_res_and_quit(HttpResponse::kStatusForbidden);
        return false;
    }

    if (!is_interpreter_cgi) {
        // Direct CGI → script itself must be executable
        if (access(path_.c_str(), X_OK) != 0) {
            set_res_and_quit(HttpResponse::kStatusForbidden);
            return false;
        }
    }
    else {
        // Interpreter CGI → interpreter must be executable
        if (access(config_.shared.cgi.exec_path.c_str(), X_OK) != 0) {
            set_res_and_quit(HttpResponse::kStatusInternalServerError);
            return false;
        }
    }
    return true;
}

static std::vector<char*> make_envp(std::vector<std::string>& env)
{
    std::vector<char*> envp;
    envp.reserve(env.size() + 1);

    for (size_t i = 0; i < env.size(); ++i)
        envp.push_back(const_cast<char*>(env[i].c_str()));

    envp.push_back(NULL);
    return envp;
}

std::vector<std::string> CgiHandler::build_env_strings() const
{
    std::vector<std::string> env;

    env.push_back("REQUEST_METHOD=" + saved_request_.method);
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("PATH_INFO=/");
    env.push_back("SCRIPT_NAME=" + saved_request_.path);
    env.push_back("QUERY_STRING=" + saved_request_.query_string);
    env.push_back("CONTENT_LENGTH=" + to_string(saved_request_.content_length));
    if (saved_request_.method == "POST") {
        env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
    }
    env.push_back("SERVER_NAME=localhost"); // to be updated based on config

    env.push_back("SERVER_PORT=" + to_string(WEBSERV_DEFAULT_PORT));
    env.push_back("PATH=/usr/bin:/bin");

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");

    env.push_back("REQUEST_URI=" + saved_request_.path +
                  (saved_request_.query_string.empty() ? "" : "?" + saved_request_.query_string));
    env.push_back("REDIRECT_STATUS=200"); // VERY IMPORTANT

    LOG(DEBUG) << "saved_request_.path = " << saved_request_.path;

    for (size_t i = 0; i < env.size(); ++i) {
        LOG(DEBUG) << "CGI ENV: " << env[i];
    }

    return (env);
}

void CgiHandler::build_exec_context(char** argv, bool is_interpreter_cgi)
{
    env_strings_ = build_env_strings();
    envp_ = make_envp(env_strings_);

    if (is_interpreter_cgi) {
        // Interpreter-based CGI
        argv[0] = const_cast<char*>(config_.shared.cgi.exec_path.c_str());
        argv[1] = const_cast<char*>(path_.c_str());
        argv[2] = NULL;
    }
    else {
        // Direct executable CGI (binary or shebang)
        argv[0] = const_cast<char*>(path_.c_str());
        argv[1] = NULL;
    }
}

bool CgiHandler::setup_pipes()
{
    if (saved_request_.method == "POST") {
        if (pipe(input_fd_) == -1) {
            set_res_and_quit(HttpResponse::kStatusInternalServerError);
            return false;
        }
    }
    if (pipe(output_fd_) == -1) {
        set_res_and_quit(HttpResponse::kStatusInternalServerError);
        return false;
    }
    return true;
}

void CgiHandler::spawn_child(char** argv)
{
    pid_ = fork();
    if (pid_ == -1) {
        set_res_and_quit(HttpResponse::kStatusInternalServerError);
        return;
    }

    if (pid_ == 0) { // child process - setting up pipes
        // closing the write end and replacing STDIN by the read end of the input pipe
        if (saved_request_.method == "POST") {
            close(input_fd_[1]);
            dup2(input_fd_[0], STDIN_FILENO);
            close(input_fd_[0]); // as now duplicated to STDIN
        }
        // closing the read end and replacing STDOUT by the write end of the output pipe
        close(output_fd_[0]);
        dup2(output_fd_[1], STDOUT_FILENO);
        close(output_fd_[1]); // as now duplicated to STDOUT
        execve(argv[0], argv, envp_.data());
        // execve(argv[0], argv, envp.data());
        _exit(1);
    }
    else {
        // parent process - setting up pipes
        // closing read end of input_fd
        if (saved_request_.method == "POST") {
            close(input_fd_[0]);
            fcntl(input_fd_[1], F_SETFL, O_NONBLOCK);
        }
        // closing write end of the output_fd
        close(output_fd_[1]);
        fcntl(output_fd_[0], F_SETFL, O_NONBLOCK);
    }
}

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
      eob_reached_(false),
      eof_reached_(false),
      eoo_reached_(false),
      forced_response_(false),
      child_reaped_(false),
      pid_(-1)
{
    // STEP 0 - Set pipe default values
    init_state();

    // STEP 1 - Check that file exisst and is executable
    bool is_interpreter_cgi = !config_.shared.cgi.exec_path.empty();
    if (validate_cgi_target(is_interpreter_cgi) != true)
        return;

    // STEP 2 - prepare environment variables and argv for child process
    char* argv[3];
    build_exec_context(argv, is_interpreter_cgi);

    // STEP 3 - set up pipes
    if (setup_pipes() != true)
        return;

    // STEP 4 - Fork child process
    spawn_child(argv);
}

CgiHandler::~CgiHandler()
{
    if (input_fd_[1] != -1) {
        close(input_fd_[1]);
        input_fd_[1] = -1;
    }
    if (output_fd_[0] != -1) {
        close(output_fd_[0]);
        output_fd_[0] = -1;
    }
}

void CgiHandler::set_res_and_quit(HttpResponse::Status status)
{
    headers_ = HttpResponse::make_error(status).to_string();
    headers_parsed_ = true;
    headers_sent_ = false;
    headers_off_ = 0;
    forced_response_ = true;
}

bool CgiHandler::child_reaped(void) const
{
    if (child_reaped_) {
        return true;
    }
    if (pid_ < 0) {
        return true;
    }
    int status = 0;
    pid_t r = waitpid(pid_, &status, WNOHANG);
    if (r == 0) {
        return false;
    }
    if (r == pid_) {
        child_reaped_ = true;
        return true;
    }
    if (r == -1) {
        child_reaped_ = true;
        return true;
    }
    return false;
}

size_t CgiHandler::write_data(const char* buf, size_t n)
{
    ssize_t bytes = write(input_fd_[1], buf, n);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { // pipe not available for write
            return 0;
        }
        eob_reached_ = true;
        if (headers_.empty()) {
            set_res_and_quit(HttpResponse::kStatusInternalServerError);
            bytes_written_body_ = body_length_; // to ensure needs_input returns false
        }
        return 0;
    }
    bytes_written_body_ += bytes;
    if (bytes_written_body_ >= body_length_) {
        close(input_fd_[1]);
        input_fd_[1] = -1;
    }
    return (bytes);
}

bool CgiHandler::has_output() const
{
    if (headers_parsed_ && !headers_sent())
        return true;
    if (output_body_off_ < output_body_.size())
        return true;
    return false;
}

size_t CgiHandler::read_data(char* buf, size_t n)
{
    if (headers_parsed_ && !headers_sent()) {
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

    ssize_t bytes_read = read(output_fd_[0], tmp, sizeof(tmp));
    LOG(DEBUG) << "bytes_read = " << bytes_read;
    if (bytes_read == 0) {
        if (!headers_parsed_) {
            set_res_and_quit(HttpResponse::kStatusBadGateway);
            return 0;
        }
        eof_reached_ = true;
    }
    else if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            if (!headers_parsed_) {
                set_res_and_quit(HttpResponse::kStatusBadGateway);
            }
            eoo_reached_ = true;
        }
    }
    else {
        if (!headers_parsed_) {
            raw_output_.append(tmp, bytes_read);

            size_t header_end = raw_output_.find("\r\n\r\n");
            size_t sep_len = 4;

            if (header_end == std::string::npos) {
                header_end = raw_output_.find("\n\n");
                sep_len = 2;
            }
            if (header_end != std::string::npos) {
                std::string cgi_headers = raw_output_.substr(0, header_end);
                std::string remainder = raw_output_.substr(header_end + sep_len);

                HttpResponse res;
                if (parse_headers(cgi_headers, res) != true) {
                    set_res_and_quit(HttpResponse::kStatusBadGateway);
                    return (0);
                }
                else {
                    // Build real HTTP headers (your to_string should include content-type, etc.)
                    headers_ = res.to_string();
                    headers_parsed_ = true;
                    headers_sent_ = false;
                    headers_off_ = 0;

                    // Buffer any bytes that were already part of body
                    output_body_.append(remainder);
                }
                // Clear raw buffer (we don't need it anymore once headers are parsed)
                raw_output_.clear();
            }
        }
        else {
            output_body_.append(tmp, bytes_read);
        }
    }
    if (eof_reached_ == true && child_reaped() == true) {
        eoo_reached_ = true;
    }
    return 0;
}

bool CgiHandler::parse_headers(std::string& cgi_headers, HttpResponse& res)
{
    int status_code = 200;                           // = default
    size_t pos_status = cgi_headers.find("Status:"); // extract Status
    if (pos_status != std::string::npos) {
        size_t line_end = cgi_headers.find("\n", pos_status);
        std::string st_line = cgi_headers.substr(pos_status, line_end - pos_status);
        int code = atoi(st_line.c_str() + 7);
        if (code > 0)
            status_code = code;
    }

    std::string content_type = ""; // extract Content-Type
    size_t pos_content = cgi_headers.find("Content-Type:");
    if (pos_content != std::string::npos) {
        size_t line_end = cgi_headers.find("\n", pos_content);
        std::string content_line =
            cgi_headers.substr(pos_content + 14, line_end - (pos_content + 14));
        str_trim(content_line);
        content_type = content_line;
    }
    // CGI spec: missing Content-Type → error
    if (content_type.empty()) {
        return false;
    }

    int content_length = 0;
    size_t pos_length = cgi_headers.find("Content-Length:");
    if (pos_length != std::string::npos) {
        size_t line_end = cgi_headers.find("\n", pos_length);
        std::string length_line = cgi_headers.substr(pos_length + 15, line_end - (pos_length + 15));
        str_trim(length_line);
        content_length = atoi(length_line.c_str());
    }

    res.code = HttpResponse::status_from_int(status_code);
    res.content_type = content_type;
    if (content_length > 0) {
        res.content_length = content_length;
    };
    // LOG(DEBUG) << res.to_string();
    return (true);
}
