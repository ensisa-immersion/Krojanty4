#ifndef CLIENT_H
#define CLIENT_H

int client(const char *ip_address, int port);
void send_message(int client_socket, char *message);

#endif //CLIENT_H