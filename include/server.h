#ifndef SERVER_H
#define SERVER_H

#include "game.h"

int run_server_1v1(Game *game, int port);
int run_server_host(Game *game, int port);

#endif
