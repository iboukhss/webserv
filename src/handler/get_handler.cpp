#include "get_handler.hpp"

#include "sys/stat.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>

#define BUFFER_SIZE 4096

// check if file isOpen else continue stream to message_buffer
void GetHandler::on_writable(Client* conn)
{
    char buff[BUFFER_SIZE];
    int bytes_read = read(fd_, buff, BUFFER_SIZE);
    if (bytes_read > 0) {
        (conn->send_buffer()).append(buff, bytes_read);
    }
    else if (bytes_read == 0) {
        done_ = true;
        close(fd_);
    }
    else if (errno != EAGAIN && errno != EINTR) {
        done_ = true;
        close(fd_);
        // conn->res().set_error(500, "Read Error");
    }
}

std::string GetHandler::derive_file_type()
{
    size_t pos = path_.rfind(".");
    if (pos == std::string::npos)
        return ("application/octet-stream");
    std::string ext = path_.substr(pos + 1);
    if (ext == "html" || ext == "htm")
        return "text/html";
    else if (ext == "txt")
        return "text/plain";
    else if (ext == "css")
        return "text/css";
    else if (ext == "js")
        return "application/javascript";
    else if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    else if (ext == "png")
        return "image/png";
    else if (ext == "gif")
        return "image/gif";
    else if (ext == "ico")
        return "image/x-icon";
    else
        return "application/octet-stream";
}

void GetHandler::write_headers(Client* conn)
{
    HttpResponse& res = conn->res();
    res.status = 200;
    res.headers["Content-Type"] = derive_file_type();
    std::ostringstream oss;
    oss << size_;
    res.headers["Content-Length"] = oss.str();
    res.headers["Connection"] = "keep-alive";
    (conn->send_buffer()).append(res.to_string());
}

GetHandler::GetHandler(Client* conn, const std::string& path)
    : Handler(path),
      fd_(-1),
      size_(0)
// done_(false)
{
    struct stat file_stat;

    if (stat(path.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        // conn.set_status(kNotFound);
        //  how to stop then here ?
        return;
    }
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ == -1) {
        // file could not be opened
        // conn.set_status(kInternalServerError);
        return;
    }
    size_ = file_stat.st_size;
    write_headers(conn);
}

GetHandler::~GetHandler()
{
    if (fd_ != -1)
        close(fd_);
}
