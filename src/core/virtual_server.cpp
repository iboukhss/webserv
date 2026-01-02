#include "core/virtual_server.hpp"

VirtualServer::VirtualServer(const ServerConfig& config)
    : config_(config),
      router_(config.locations)
{
}
