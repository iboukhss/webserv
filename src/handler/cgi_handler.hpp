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
    CgiHandler(const std::string& path, const HttpRequest& request, const ServerConfig& config);
    virtual ~CgiHandler();

    virtual size_t read_data(char* buf, size_t n);
    virtual size_t write_data(const char* buf, size_t n);

    void CgiHandler::setEnvVar(std::vector<std::string>& child_env_var);
    int CgiHandler::childReaped(void);
    int CgiHandler::parse_headers(std::string& cgi_headers, HttpResponse& res);

    virtual bool has_output() const { return !child_reaped_ || !headers_sent() || !eoo_reached_; }
    virtual bool needs_input() const { return bytes_written_body_ < body_length_; };
    virtual bool is_done() const { return !has_output(); }
    const std::string& path() const { return path_; }

private:
    CgiHandler(const CgiHandler&);
    CgiHandler& operator=(const CgiHandler&);

    // bool has_body() const { return body_length_ > 0; }
    bool headers_sent() const { return headers_off_ == headers_.size(); }
    bool body_written_to_STDIN() const { return eob_reached_; }

    const std::string path_;

    const HttpRequest& saved_request_;
    const ServerConfig& config_;

    // POST/GET - strings used to parse the cgi output
    std::string raw_output_;
    off_t headers_off_;
    std::string headers_;
    off_t output_body_off_;
    std::string output_body_;

    // POST - body written to write end of the input_fd pipe
    off_t bytes_written_body_;
    size_t body_length_;

    // boolean
    bool headers_parsed_;
    bool headers_sent_;
    bool eob_reached_;
    bool eoo_reached_;
    bool child_reaped_;

    // pipes
    int input_fd[2];
    int output_fd[2];

    // child process pid
    pid_t pid_;
};

#endif
