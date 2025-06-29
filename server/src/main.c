#include "server.h"
#include "signal.h"
#include "utils.h"
#include <stdio.h>

int main(int argc, char **argv) {
    server_t srv;

    if (server_init(&srv, argc, argv) != 0) {
        die("Échec de l'initialisation du serveur");
    }

    log_info("Serveur démarré sur le port %u (%dx%d)", srv.port, srv.width, srv.height);
    signal(SIGINT, handle_signal);  // Ctrl+C
    signal(SIGTERM, handle_signal);
    server_run(&srv);
    server_cleanup(&srv);
    return 0;
}
