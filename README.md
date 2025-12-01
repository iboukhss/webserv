# Webserv

A simple HTTP server written in C++98 using epoll (Linux-only)

## Features

Supported HTTP versions

- HTTP/1.0 - decent support
- HTTP/1.1 - limited support

HTTP methods:

- GET
- POST
- DELETE

Server capabilities:

- Non-blocking I/O with level-triggered `epoll`
- Chunked transfer encoding
- Persistent connections (`Connection: keep-alive`)
- CGI support

## Build instructions

Because this project relies on Linux-specific `epoll` it is not compatible with other operating systems.

```sh
make
```

Executable can be found under `/build/config_name/bin/webserv`.

## Usage

Example configurations files can be found under the `config/` directory. Format is inspired by NGINX.

```sh
./webserv path/to/config
```

## Benchmarks

Add here

## Development dependencies

- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) - code formatting
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) - code analysis
- [compiledb](https://github.com/nickdiego/compiledb) - generates compilation database for clang tooling
- [utest.h](https://github.com/sheredom/utest.h) - lightweight unit test framework inspired by googletest (C++98 compatible)
