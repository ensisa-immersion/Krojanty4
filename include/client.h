#ifndef CLIENT_H
#define CLIENT_H

#include "game.h"

extern int g_client_socket;

int connect_to_server(const char *ip, int port);
int start_client_rx(Game *game);
void send_message(int client_socket, const char *move4);
void client_close(void);

#endif
