
#ifndef COOKIE_LIBINIT_H
#define COOKIE_LIBINIT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBINIT_SOCKET_PATH "/run/cookie-init.sock"

enum libinit_request_type {
    LIBINIT_REQUEST_RESTART = 1,
    LIBINIT_REQUEST_SHUTDOWN = 2,
    LIBINIT_REQUEST_REBOOT = 3
};

/*
 * Client API.
 *
 * Returns 0 on success, -1 on error with errno set.
 */
int libinit_request(enum libinit_request_type request);

int libinit_request_restart(void);
int libinit_request_shutdown(void);
int libinit_request_reboot(void);

/*
 * Server API.
 *
 * Open the control socket, then call libinit_server_serve_one()
 * repeatedly to handle incoming requests.
 */
typedef int (*libinit_server_handler)(
    enum libinit_request_type request,
    uid_t uid,
    pid_t pid,
    void *userdata
);

int libinit_server_open(void);

int libinit_server_serve_one(
    int server_fd,
    libinit_server_handler handler,
    void *userdata
);

void libinit_server_close(int server_fd);

#ifdef __cplusplus
}
#endif

#endif
