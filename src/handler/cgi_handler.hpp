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
    CgiHandler(const std::string& path, const HttpRequest& request, const RouteConfig& config);
    virtual ~CgiHandler();

    virtual size_t read_data(char* buf, size_t n);
    virtual size_t write_data(const char* buf, size_t n);

    void set_env_var(std::vector<std::string>& child_env_var);
    int child_reaped(void) const;
    int parse_headers(std::string& cgi_headers, HttpResponse& res);

    virtual bool has_output() const;
    virtual bool needs_input() const
    {
        return input_fd_[1] != -1 && bytes_written_body_ < body_length_;
    };
    virtual bool is_done() const
    {
        child_reaped();
        return eoo_reached_ && !needs_input() && child_reaped_ && !has_output();
    }
    const std::string& path() const { return path_; }

    virtual int cgi_read_fd() const { return output_fd_[0]; };
    virtual int cgi_write_fd() const { return input_fd_[1]; };

private:
    CgiHandler(const CgiHandler&);
    CgiHandler& operator=(const CgiHandler&);

    // bool has_body() const { return body_length_ > 0; }
    bool headers_sent() const { return headers_off_ == headers_.size(); }
    // bool body_written_to_STDIN() const { return eob_reached_; }

    const std::string path_;

    const HttpRequest& saved_request_;
    const RouteConfig& config_;

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
    bool eoo_reached_;
    mutable bool child_reaped_;

    // pipes
    int input_fd_[2];
    int output_fd_[2];

    // child process pid
    mutable pid_t pid_;
};

#endif
