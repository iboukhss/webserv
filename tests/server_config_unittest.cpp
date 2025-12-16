#include "config/server_config.hpp"
#include "utest/utest.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdexcept>

UTEST(ServerConfigTest, ListenPortOnly)
{
    ServerConfig srv;
    srv.add_listen_addr("8080");

    ASSERT_EQ(1u, srv.listen_addrs.size());

    const sockaddr_in& addr = srv.listen_addrs[0];
    EXPECT_EQ(8080, ntohs(addr.sin_port));
    EXPECT_EQ(INADDR_ANY, ntohl(addr.sin_addr.s_addr));
}

UTEST(ServerConfigTest, ListenIpOnly)
{
    ServerConfig srv;
    srv.add_listen_addr("127.0.0.1");

    ASSERT_EQ(1u, srv.listen_addrs.size());

    const sockaddr_in& addr = srv.listen_addrs[0];
    EXPECT_EQ(8080, ntohs(addr.sin_port));

    in_addr expected;
    ASSERT_EQ(1, inet_pton(AF_INET, "127.0.0.1", &expected));
    EXPECT_EQ(expected.s_addr, addr.sin_addr.s_addr);
}

UTEST(ServerConfigTest, ListenIpPort)
{
    ServerConfig srv;
    srv.add_listen_addr("127.0.0.1:4242");

    ASSERT_EQ(1u, srv.listen_addrs.size());

    const sockaddr_in& addr = srv.listen_addrs[0];
    EXPECT_EQ(4242, ntohs(addr.sin_port));

    in_addr expected;
    ASSERT_EQ(1, inet_pton(AF_INET, "127.0.0.1", &expected));
    EXPECT_EQ(expected.s_addr, addr.sin_addr.s_addr);
}

UTEST(ServerConfigTest, ListenAnyPort)
{
    ServerConfig srv;
    srv.add_listen_addr(":1234");

    ASSERT_EQ(1u, srv.listen_addrs.size());

    const sockaddr_in& addr = srv.listen_addrs[0];
    EXPECT_EQ(1234, ntohs(addr.sin_port));
    EXPECT_EQ(INADDR_ANY, addr.sin_addr.s_addr);
}

UTEST(ServerConfigTest, ListenDefaultPort)
{
    ServerConfig srv;
    srv.add_listen_addr("1.2.3.4:");

    ASSERT_EQ(1u, srv.listen_addrs.size());

    const sockaddr_in& addr = srv.listen_addrs[0];
    EXPECT_EQ(8080, ntohs(addr.sin_port));

    in_addr expected;
    ASSERT_EQ(1, inet_pton(AF_INET, "1.2.3.4", &expected));
    EXPECT_EQ(expected.s_addr, addr.sin_addr.s_addr);
}

UTEST(ServerConfigTest, InvalidListenAddrThrows)
{
    ServerConfig srv;

    EXPECT_EXCEPTION(srv.add_listen_addr(""), std::runtime_error);
    EXPECT_EXCEPTION(srv.add_listen_addr("abc"), std::runtime_error);
    EXPECT_EXCEPTION(srv.add_listen_addr("1.2.3"), std::runtime_error);
    EXPECT_EXCEPTION(srv.add_listen_addr("999.1.1.1"), std::runtime_error);
    EXPECT_EXCEPTION(srv.add_listen_addr("127.0.0.1:0"), std::runtime_error);
    // EXPECT_EXCEPTION(srv.add_listen_addr("01.01.01.01"), std::runtime_error);
}
