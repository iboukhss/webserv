#include "get_handler.hpp"

#include "sys/stat.h"

#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

// check if file isOpen else continue stream to message_buffer
void GetHandler::on_writable(Client* conn)
{
    char buff[BUFFER_SIZE];
    int bytes_read = read(fd_, buff, BUFFER_SIZE);
    buff[bytes_read] = '\0';
    if (bytes_read > 0) {
        conn->append_send_buffer(buff);
    }
    else if (bytes_read == 0) {
        done_ = true;
    }
    else {
        // error handling
    }
}

GetHandler::GetHandler(const Client* conn, const std::string& path)
    : Handler(path)
{
    struct stat file_stat;

    if (stat(path.c_str(), &file_stat) != 0) {
        // conn.set_status(kNotFound);
        //  how to stop then here ?
    }
    if (!S_ISREG(file_stat.st_mode)) {
        // not a file -> how to handle ?
        // conn.set_status(kBadRequest);
    }
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ == -1) {
        // file could not be opened
        // conn.set_status(kInternalServerError);
    }
}

GetHandler::~GetHandler()
{
    if (fd_ != -1)
        close(fd_);
}
