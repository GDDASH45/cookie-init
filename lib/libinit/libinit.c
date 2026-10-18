
#define _GNU_SOURCE

#include "libinit.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define LIBINIT_MAGIC   0x43494E49U
#define LIBINIT_VERSION 1U

struct libinit_message {
    uint32_t magic;
    uint16_t version;
    uint16_t request;
    int32_t status;
};

static int connect_socket(void)
{
    int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (fd == -1)
        return -1;

    struct sockaddr_un address = {0};
    address.sun_family = AF_UNIX;

    if (strlen(LIBINIT_SOCKET_PATH) >= sizeof(address.sun_path)) {
        close(fd);
        errno = ENAMETOOLONG;
        return -1;
    }

    strcpy(address.sun_path, LIBINIT_SOCKET_PATH);

    if (connect(fd, (struct sockaddr *)&address,
                sizeof(address)) == -1) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    return fd;
}

int libinit_request(enum libinit_request_type request)
{
    if (request < LIBINIT_REQUEST_RESTART ||
        request > LIBINIT_REQUEST_REBOOT) {
        errno = EINVAL;
        return -1;
    }

    int fd = connect_socket();
    if (fd == -1)
        return -1;

    struct libinit_message message = {
        .magic = LIBINIT_MAGIC,
        .version = LIBINIT_VERSION,
        .request = (uint16_t)request,
        .status = 0
    };

    ssize_t sent = send(fd, &message, sizeof(message), MSG_NOSIGNAL);
    if (sent != (ssize_t)sizeof(message)) {
        int saved_errno = sent == -1 ? errno : EIO;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    struct libinit_message response;
    ssize_t received = recv(fd, &response, sizeof(response), 0);

    int saved_errno = errno;
    close(fd);

    if (received != (ssize_t)sizeof(response)) {
        errno = received == -1 ? saved_errno : EPROTO;
        return -1;
    }

    if (response.magic != LIBINIT_MAGIC ||
        response.version != LIBINIT_VERSION ||
        response.request != (uint16_t)request) {
        errno = EPROTO;
        return -1;
    }

    if (response.status != 0) {
        errno = response.status;
        return -1;
    }

    return 0;
}

int libinit_request_restart(void)
{
    return libinit_request(LIBINIT_REQUEST_RESTART);
}

int libinit_request_shutdown(void)
{
    return libinit_request(LIBINIT_REQUEST_SHUTDOWN);
}

int libinit_request_reboot(void)
{
    return libinit_request(LIBINIT_REQUEST_REBOOT);
}

int libinit_server_open(void)
{
    int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (fd == -1)
        return -1;

    struct sockaddr_un address = {0};
    address.sun_family = AF_UNIX;

    if (strlen(LIBINIT_SOCKET_PATH) >= sizeof(address.sun_path)) {
        close(fd);
        errno = ENAMETOOLONG;
        return -1;
    }

    strcpy(address.sun_path, LIBINIT_SOCKET_PATH);

    /*
     * The socket lives in /run, which should already be mounted
     * by Cookie Init. Only remove the stale socket pathname.
     */
    if (unlink(LIBINIT_SOCKET_PATH) == -1 && errno != ENOENT) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    if (bind(fd, (struct sockaddr *)&address,
             sizeof(address)) == -1) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    /*
     * Restrict access to root by default. Change this policy
     * deliberately if non-root clients should be supported.
     */
    if (chmod(LIBINIT_SOCKET_PATH, S_IRUSR | S_IWUSR) == -1) {
        int saved_errno = errno;
        close(fd);
        unlink(LIBINIT_SOCKET_PATH);
        errno = saved_errno;
        return -1;
    }

    if (listen(fd, 16) == -1) {
        int saved_errno = errno;
        close(fd);
        unlink(LIBINIT_SOCKET_PATH);
        errno = saved_errno;
        return -1;
    }

    return fd;
}

int libinit_server_serve_one(
    int server_fd,
    libinit_server_handler handler,
    void *userdata)
{
    if (server_fd < 0 || handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    int client_fd;
    do {
        client_fd = accept4(server_fd, NULL, NULL, SOCK_CLOEXEC);
    } while (client_fd == -1 && errno == EINTR);

    if (client_fd == -1)
        return -1;

    int result = -1;
    struct libinit_message request = {0};
    struct libinit_message response = {0};

    struct ucred credentials;
    socklen_t credentials_size = sizeof(credentials);

    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED,
                   &credentials, &credentials_size) == -1)
        goto done;

    ssize_t received = recv(client_fd, &request, sizeof(request), 0);
    if (received != (ssize_t)sizeof(request)) {
        errno = received == -1 ? errno : EPROTO;
        goto done;
    }

    response.magic = LIBINIT_MAGIC;
    response.version = LIBINIT_VERSION;
    response.request = request.request;

    if (request.magic != LIBINIT_MAGIC ||
        request.version != LIBINIT_VERSION) {
        response.status = EPROTO;
    } else if (request.request < LIBINIT_REQUEST_RESTART ||
               request.request > LIBINIT_REQUEST_REBOOT) {
        response.status = EINVAL;
    } else if (credentials.uid != 0) {
        response.status = EPERM;
    } else {
        errno = 0;

        int handler_result = handler(
            (enum libinit_request_type)request.request,
            credentials.uid,
            credentials.pid,
            userdata
        );

        if (handler_result == -1)
            response.status = errno ? errno : EIO;
        else
            response.status = 0;
    }

    {
        ssize_t sent = send(client_fd, &response,
                            sizeof(response), MSG_NOSIGNAL);
        if (sent != (ssize_t)sizeof(response)) {
            if (sent >= 0)
                errno = EIO;
            goto done;
        }
    }

    result = 0;

done:
    {
        int saved_errno = errno;
        close(client_fd);
        errno = saved_errno;
    }

    return result;
}

void libinit_server_close(int server_fd)
{
    if (server_fd >= 0)
        close(server_fd);

    unlink(LIBINIT_SOCKET_PATH);
}
