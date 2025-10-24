#include "Server.hpp"

int main()
{
    Server server(INADDR_ANY, 8080);

    server.run();

    return 0;
}
