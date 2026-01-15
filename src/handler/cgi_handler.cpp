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

CgiHandler::CgiHandler(const std::string& path, const RouteConfig& rc, const HttpRequest& req)
    : path_(path),
      rc_(rc),
      req_(req),
      out_off_(0),
      output_body_off_(0),
      bytes_written_body_(0),
      body_length_(req.content_length),
      headers_parsed_(false),
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
    bool is_interpreter_cgi = !rc.shared.cgi.exec_path.empty();
    if (validate_cgi_target(is_interpreter_cgi) != true)
        return;

    // STEP 2 - prepare environment variables and argv for child process
    char* argv[3];
    build_exec_context(argv, is_interpreter_cgi);
    LOG(DEBUG) << "CgiHandler : exec context built";
    // STEP 3 - set up pipes
    if (setup_pipes() != true)
        return;
    LOG(DEBUG) << "CgiHandler : pipes setup";
    // STEP 4 - Fork child process
    spawn_child(argv);
    LOG(DEBUG) << "CGI HANDLER CONSTRUCTOR";
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
        LOG(DEBUG) << "CgiHandler: file does not exist -> " << path_;
        set_error(HttpResponse::kStatusForbidden);
        return false;
    }

    if (!is_interpreter_cgi) {
        // Direct CGI → script itself must be executable
        if (access(path_.c_str(), X_OK) != 0) {
            LOG(DEBUG) << "CgiHandler: script not executable";
            set_error(HttpResponse::kStatusForbidden);
            return false;
        }
    }
    else {
        // Interpreter CGI → interpreter must be executable
        if (access(rc_.shared.cgi.exec_path.c_str(), X_OK) != 0) {
            LOG(DEBUG) << "CgiHandler: interpreter not executable";
            set_error(HttpResponse::kStatusInternalServerError);
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

    env.push_back("REQUEST_METHOD=" + req_.method);
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("PATH_INFO=/");
    env.push_back("SCRIPT_NAME=" + req_.path);
    env.push_back("QUERY_STRING=" + req_.query_string);
    env.push_back("CONTENT_LENGTH=" + to_string(req_.content_length));
    if (req_.method == "POST") {
        env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
    }
    env.push_back("SERVER_NAME=localhost"); // to be updated based on config

    env.push_back("SERVER_PORT=" + to_string(WEBSERV_DEFAULT_PORT));
    env.push_back("PATH=/usr/bin:/bin");

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");

    env.push_back("REQUEST_URI=" + req_.path +
                  (req_.query_string.empty() ? "" : "?" + req_.query_string));
    env.push_back("REDIRECT_STATUS=200"); // VERY IMPORTANT

    LOG(DEBUG) << "req_.path = " << req_.path;

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
        argv[0] = const_cast<char*>(rc_.shared.cgi.exec_path.c_str());
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
    if (req_.method == "POST") {
        if (pipe(input_fd_) == -1) {
            set_error(HttpResponse::kStatusInternalServerError);
            return false;
        }
    }
    if (pipe(output_fd_) == -1) {
        set_error(HttpResponse::kStatusInternalServerError);
        return false;
    }
    return true;
}

void CgiHandler::spawn_child(char** argv)
{
    pid_ = fork();
    if (pid_ == -1) {
        set_error(HttpResponse::kStatusInternalServerError);
        return;
    }

    if (pid_ == 0) { // child process - setting up pipes
        // closing the write end and replacing STDIN by the read end of the input pipe
        if (req_.method == "POST") {
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
        if (req_.method == "POST") {
            close(input_fd_[0]);
            fcntl(input_fd_[1], F_SETFL, O_NONBLOCK);
        }
        // closing write end of the output_fd
        close(output_fd_[1]);
        fcntl(output_fd_[0], F_SETFL, O_NONBLOCK);
    }
}

void CgiHandler::set_error(const HttpResponse::Status code)
{
    // Build error response
    res_ = HttpResponse::make_error(code, rc_.shared.error_pages, req_);
    out_buf_ = res_.to_string();

    forced_response_ = true;

    // Stop waiting for request body input
    bytes_written_body_ = body_length_;

    // Mark output lifecycle as ended (so is_done can succeed once out_buf drained)
    eof_reached_ = true;
    eoo_reached_ = true;
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

size_t CgiHandler::write_input(const char* buf, size_t n)
{
    if (forced_response_ || input_fd_[1] == -1) {
        // We no longer accept input (error response or already finished input)
        return 0;
    }

    ssize_t bytes = write(input_fd_[1], buf, n);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }

        // Real error writing to CGI stdin
        eob_reached_ = true;
        if (!forced_response_) {
            set_error(HttpResponse::kStatusInternalServerError);
        }

        // Stop waiting for request body input
        bytes_written_body_ = body_length_;
        // Optionally mark input closed in a way needs_input() respects:
        // input_fd_[1] = -1;  // (but do NOT close here if client caches fds)

        return 0;
    }

    bytes_written_body_ += static_cast<size_t>(bytes);

    if (bytes_written_body_ >= body_length_) {
        close(input_fd_[1]);
        input_fd_[1] = -1;
        bytes_written_body_ = body_length_;
    }

    return static_cast<size_t>(bytes);
}

bool CgiHandler::is_done() const
{
    // If we forced an error/response, we're done once we've sent it.
    if (forced_response_) {
        return out_off_ >= out_buf_.size();
    }

    // Update child state (non-blocking)
    child_reaped();

    // If CGI stdout reached EOF and child is reaped, output is over.
    if (eof_reached_ && child_reaped_) {
        eoo_reached_ = true;
    }

    // Close CGI stdout read end exactly once, but only after we've drained buffered output.
    if (eoo_reached_ && out_off_ >= out_buf_.size() && output_fd_[0] != -1) {
        close(output_fd_[0]);
        output_fd_[0] = -1;
    }

    // Final: no more output will arrive, no more input needed, and nothing buffered to send.
    return eoo_reached_ && !needs_input() && (out_off_ >= out_buf_.size());
}

ReadHdr CgiHandler::read_pipe_until_crlf_()
{
    char tmp[4096];
    while (true) {
        ssize_t bytes = read(output_fd_[0], tmp, sizeof(tmp));
        if (bytes > 0) {
            pipe_buf_.append(tmp, bytes);
            if (pipe_buf_.find("\r\n\r\n") != std::string::npos ||
                pipe_buf_.find("\n\n") != std::string::npos) {
                return kHdrComplete;
            }
            continue;
        }
        if (bytes == 0) { // EOF -> child closed stdout before headers were sent (entirely)
            return kHdrFail;
        }
        if (bytes < 0 && (errno == EAGAIN ||
                          errno == EWOULDBLOCK)) { // no more bytes right now; wait for next EPOLLIN
            return kHdrNeedMore;
        }
        return kHdrFail;
    }
}

size_t CgiHandler::send_out_buf_(char* buf, size_t n)
{
    if (out_off_ >= out_buf_.size()) {
        return 0;
    }
    size_t bytes_left = out_buf_.size() - out_off_;
    size_t to_copy = std::min(bytes_left, n);
    std::memcpy(buf, out_buf_.data() + out_off_, to_copy);
    out_off_ += to_copy;
    return (to_copy);
}

size_t CgiHandler::read_output(char* buf, size_t n)
{
    size_t sent = send_out_buf_(buf, n);
    if (sent > 0)
        return sent;

    if (!headers_parsed_) {
        ReadHdr r = read_pipe_until_crlf_();
        if (r == kHdrNeedMore) {
            return 0; // @IBOUKH : HERE I NEED TO CHECK WITH YOU HOW TO HANDLE THIS IN THE CLIENT
        }
        if (r == kHdrFail) {
            set_error(HttpResponse::kStatusBadGateway);
            return send_out_buf_(buf, n);
        }

        // r == kHdrComplete
        if (!parse_headers()) {
            set_error(HttpResponse::kStatusBadGateway);
            return send_out_buf_(buf, n);
        }

        headers_parsed_ = true; // (or parse_headers sets this)
        return send_out_buf_(buf, n);
    }
    // headers parsed: now read body
    char tmp[4096];
    while (true) {
        ssize_t bytes = read(output_fd_[0], tmp, sizeof(tmp));
        if (bytes > 0) {
            out_buf_.append(tmp, bytes);
            return send_out_buf_(buf, n);
        }
        if (bytes == 0) {
            eof_reached_ = true;
            return 0;
        }
        if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0; // @IBOUKH : HERE I NEED TO CHECK WITH YOU HOW TO HANDLE THIS IN THE CLIENT
        }
        eoo_reached_ = true; // or error flag
        return 0;
    }
}

bool CgiHandler::parse_headers()
{
    size_t header_end = pipe_buf_.find("\r\n\r\n");
    size_t sep_len = 4;

    if (header_end == std::string::npos) {
        header_end = pipe_buf_.find("\n\n");
        sep_len = 2;
    }
    if (header_end == std::string::npos) {
        return false; // not enough yet
    }

    std::string header_part = pipe_buf_.substr(0, header_end); // exclude delimiter
    std::string body_part = pipe_buf_.substr(header_end + sep_len);

    int status_code = 200;                           // = default
    size_t pos_status = header_part.find("Status:"); // extract Status
    if (pos_status != std::string::npos) {
        size_t line_end = header_part.find("\n", pos_status);
        std::string st_line = header_part.substr(pos_status, line_end - pos_status);
        int code = atoi(st_line.c_str() + 7);
        if (code > 0)
            status_code = code;
    }

    std::string content_type = ""; // extract Content-Type
    size_t pos_content = header_part.find("Content-Type:");
    if (pos_content != std::string::npos) {
        size_t line_end = header_part.find("\n", pos_content);
        std::string content_line =
            header_part.substr(pos_content + 14, line_end - (pos_content + 14));
        str_trim(content_line);
        content_type = content_line;
    }
    // CGI spec: missing Content-Type → error
    if (content_type.empty()) {
        return false;
    }

    int content_length = 0;
    size_t pos_length = header_part.find("Content-Length:");
    if (pos_length != std::string::npos) {
        size_t line_end = header_part.find("\n", pos_length);
        std::string length_line = header_part.substr(pos_length + 15, line_end - (pos_length + 15));
        str_trim(length_line);
        content_length = atoi(length_line.c_str());
    }

    res_.code = HttpResponse::status_from_int(status_code);
    res_.content_type = content_type;
    if (content_length > 0) {
        res_.content_length = content_length;
    };
    out_buf_ = res_.to_string();
    out_buf_.append(body_part);
    pipe_buf_.clear();

    return true;
}
