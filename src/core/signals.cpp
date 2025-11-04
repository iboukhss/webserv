#include "core/signals.hpp"

volatile sig_atomic_t g_sigint_received = 0;

extern "C" void handle_sigint(int sig)
{
    (void) sig;
    g_sigint_received = 1;
}

void setup_signal_handlers()
{
    struct sigaction sa;

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}
