#ifndef HANDLER_CGI_HANDLER_HPP_
#define HANDLER_CGI_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <sys/stat.h>
#include <sys/wait.h>

#include <string>

enum ReadHdr { kHdrComplete, kHdrNeedMore, kHdrFail };

class CgiHandler : public Handler {

public:
    CgiHandler(const std::string& path, const RouteConfig& rc, const HttpRequest& request);
    virtual ~CgiHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool is_regular_file() const { return false; }
    virtual bool has_output() const { return out_off_ < out_buf_.size(); };
    virtual bool needs_input() const
    {
        return input_fd_[1] != -1 && bytes_written_body_ < body_length_;
    };
    virtual bool is_done() const;

    const std::string& path() const { return path_; }

    virtual int cgi_read_fd() const { return output_fd_[0]; };
    virtual int cgi_write_fd() const { return input_fd_[1]; };

private:
    CgiHandler(const CgiHandler&);
    CgiHandler& operator=(const CgiHandler&);

    void set_error(HttpResponse::Status status);
    bool child_reaped(void) const;

    // constructor helper functions
    void init_state();
    bool validate_cgi_target(bool is_interpreter_cgi);
    std::vector<std::string> build_env_strings() const;
    void build_exec_context(char** argv, bool is_interpreter_cgi);
    bool setup_pipes();
    void spawn_child(char** argv);

    // read_output() helper functions
    ReadHdr read_pipe_until_crlf();
    bool parse_headers();
    size_t send_out_buf(char* buf, size_t n);

    // constructor args
    const std::string path_;
    const RouteConfig& rc_;
    const HttpRequest& req_;
    // Response built
    HttpResponse res_;
    // pipe output buffer
    std::string pipe_buf_;
    // Serialized response and offset
    std::string out_buf_;
    size_t out_off_;

    std::vector<char*> envp_;
    std::vector<std::string> env_strings_;

    // POST
    std::string output_body_;
    size_t output_body_off_;

    // POST - body written to write end of the input_fd pipe
    size_t bytes_written_body_;
    size_t body_length_;

    // boolean
    bool headers_parsed_;
    bool eob_reached_;         // end-of-body to mark the end of the body written to the pipe
    bool eof_reached_;         // end-of-file for reading output from pipe
    mutable bool eoo_reached_; // end-of-output
    bool forced_response_;
    mutable bool child_reaped_;

    // pipes
    int input_fd_[2];
    mutable int output_fd_[2];

    // child process pid
    mutable pid_t pid_;
};

#endif
