#ifndef HANDLER_CGI_HANDLER_HPP_
#define HANDLER_CGI_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <sys/stat.h>
#include <sys/wait.h>

#include <string>

class CgiHandler : public Handler {
public:
    CgiHandler(const std::string& path, const RouteConfig& rc, const HttpRequest& request);
    virtual ~CgiHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool has_output() const;
    virtual bool needs_input() const
    {
        return input_fd_[1] != -1 && bytes_written_body_ < body_length_;
    };
    bool is_done() const
    {
        if (forced_response_) {
            return !has_output();
        }
        // Progress child lifecycle
        child_reaped();

        // Close CGI stdout pipe exactly once, after draining everything
        if (!forced_response_ && eoo_reached_ && headers_sent_ && output_body_.empty() &&
            output_fd_[0] != -1) {
            close(output_fd_[0]);
            output_fd_[0] = -1;
        }

        // Final completion condition
        return eoo_reached_ && !needs_input() && child_reaped_ && !has_output();
    }
    const std::string& path() const { return path_; }

    virtual int cgi_read_fd() const { return output_fd_[0]; };
    virtual int cgi_write_fd() const { return input_fd_[1]; };

private:
    CgiHandler(const CgiHandler&);
    CgiHandler& operator=(const CgiHandler&);

    void set_res_and_quit(HttpResponse::Status status);
    std::vector<std::string> build_env_strings() const;
    bool child_reaped(void) const;
    bool parse_headers(std::string& cgi_headers, HttpResponse& res);
    bool headers_sent() const { return headers_off_ == headers_.size(); }

    // constructor helper functions
    void init_state();
    bool validate_cgi_target(bool is_interpreter_cgi);
    void build_exec_context(char** argv, bool is_interpreter_cgi);
    bool setup_pipes();
    void spawn_child(char** argv);

    // constructor args
    const std::string path_;
    const RouteConfig& rc_;
    const HttpRequest& req_;
    // Response built
    HttpResponse res_;
    // Serialized response and offset
    std::string out_buf_;
    size_t out_off_;

    std::vector<char*> envp_;
    std::vector<std::string> env_strings_;

    // POST/GET - strings used to parse the cgi output
    std::string raw_output_;
    size_t headers_off_;
    std::string headers_;
    size_t output_body_off_;
    std::string output_body_;

    // POST - body written to write end of the input_fd pipe
    size_t bytes_written_body_;
    size_t body_length_;

    // boolean
    bool headers_parsed_;
    bool headers_sent_;
    bool eob_reached_;
    bool eof_reached_;
    bool eoo_reached_;
    bool forced_response_;
    mutable bool child_reaped_;

    // pipes
    int input_fd_[2];
    mutable int output_fd_[2];

    // child process pid
    mutable pid_t pid_;
};

#endif
