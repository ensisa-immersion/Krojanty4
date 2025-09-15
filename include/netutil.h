#ifndef NETUTIL_H
#define NETUTIL_H

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stddef.h>

static inline int send_all(int fd, const void *buf, size_t len) {
    const char *p = (const char*)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, p + off, len - off, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)n;
    }
    return 0;
}

/* Lit exactement len octets (gère fragmentations/EINTR).
 * Retour: 1 succès, 0 socket fermé proprement, -1 erreur (errno défini).
 */
static inline int read_exact(int fd, void *buf, size_t len) {
    char *p = (char*)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = recv(fd, p + off, len - off, 0);
        if (n == 0) return 0;          /* fermé par pair */
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)n;
    }
    return 1;
}

#endif /* NETUTIL_H */
