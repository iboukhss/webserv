#ifndef CORE_SIGNALS_HPP_
#define CORE_SIGNALS_HPP_

#include <csignal>

extern volatile sig_atomic_t g_sigint_received;

void setup_signal_handlers();

#endif // CORE_SIGNALS_HPP_
