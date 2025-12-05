#include "handler/delete_handler.hpp"

#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

DeleteHandler::DeleteHandler(const std::string& path)
    : file_path_(path),
      headers_off_(0)
{
    struct stat file_stat;
    HttpResponse res;

    LOG(DEBUG) << "Trying to delete file : " << path;

    if (stat(path.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        res.code = HttpResponse::kStatusNotFound;
        res.content_type = "text/html; charset=UTF-8"; // Magic to display emojis
        res.inline_body = "<h1>404 Not Found 😢</h1>";
        headers_ = res.to_string();
        return;
    }

    int n = remove(path.c_str());
    if (n == -1) {
        res.code = HttpResponse::kStatusInternalServerError;
        headers_ = res.to_string();
        return;
    }
    if (file_stat.st_size == 0) {
        res.code = HttpResponse::kStatusNoContent;
        res.content_type = "text/html; charset=UTF-8"; // Magic to display emojis
        res.inline_body = "<h1>204 No Content</h1>";
        headers_ = res.to_string();
    }

    res.code = HttpResponse::kStatusOk;
    res.content_type = "text/html; charset=UTF-8"; // Magic to display emojis
    res.inline_body = "<h1>File " + path.substr(path.rfind("/"), path.size() - path.rfind("/")) +
                      " deleted.</h1>";
    headers_ = res.to_string();
}

DeleteHandler::~DeleteHandler()
{
}

size_t DeleteHandler::read_data(char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}

// We never write to this handler (read-only)
size_t DeleteHandler::write_data(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}
