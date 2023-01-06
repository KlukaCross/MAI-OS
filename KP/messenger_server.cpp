#include "server/server.h"
#include <signal.h>

int main() {
    auto *server = new Server();
    signal(SIGPIPE, SIG_IGN);
    server->start();
    delete server;
    return 0;
}