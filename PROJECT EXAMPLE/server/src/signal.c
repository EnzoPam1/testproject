#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include "server.h"

volatile sig_atomic_t stop_server = 0;

void handle_signal(int sig)
{
    (void)sig;
    stop_server = 1;
}
