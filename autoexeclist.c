
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// autoexeclist.h
#include "autoexeclist.h"

#define AUTOEXEC_LIST "/etc/cookie/autoexec.list"
#define MAX_LINE 4096

static char *trim(char *str)
{
    while (*str == ' ' || *str == '\t' ||
           *str == '\r' || *str == '\n')
        str++;

    size_t len = strlen(str);

    while (len > 0 &&
           (str[len - 1] == ' ' ||
            str[len - 1] == '\t' ||
            str[len - 1] == '\r' ||
            str[len - 1] == '\n')) {
        str[--len] = '\0';
    }

    return str;
}

static int execute_program(const char *path)
{
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "autoexeclist: fork: %s\n",
                strerror(errno));
        return -1;
    }

    if (pid == 0) {
        execl(path, path, (char *)NULL);

        fprintf(stderr, "autoexeclist: cannot execute %s: %s\n",
                path, strerror(errno));
        _exit(127);
    }

    int status;

    for (;;) {
        if (waitpid(pid, &status, 0) != -1)
            break;

        if (errno == EINTR)
            continue;

        fprintf(stderr, "autoexeclist: waitpid: %s\n",
                strerror(errno));
        return -1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);

        fprintf(stderr, "autoexeclist: %s exited with status %d\n",
                path, code);

        return code;
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr, "autoexeclist: %s terminated by signal %d\n",
                path, WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }

    return -1;
}

/*
 * Run programs from the list sequentially.
 *
 * Returns 0 if the list was processed.
 * Returns -1 if the list could not be read.
 */
int autoexeclist_run(const char *list_path)
{
    FILE *file = fopen(list_path, "r");

    if (file == NULL) {
        fprintf(stderr, "autoexeclist: cannot open %s: %s\n",
                list_path, strerror(errno));
        return -1;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    unsigned long line_number = 0;

    while ((length = getline(&line, &capacity, file)) != -1) {
        (void)length;
        line_number++;

        char *path = trim(line);

        if (*path == '\0' || *path == '#')
            continue;

        fprintf(stderr, "autoexeclist: starting %s\n", path);

        int status = execute_program(path);

        if (status == -1) {
            fprintf(stderr,
                    "autoexeclist: error on line %lu; continuing\n",
                    line_number);
        }

        /*
         * Nonzero program exit statuses are logged but do not
         * prevent later entries from running.
         */
    }

    int read_error = ferror(file);

    free(line);
    fclose(file);

    if (read_error) {
        fprintf(stderr, "autoexeclist: error reading %s\n",
                list_path);
        return -1;
    }

    return 0;
}

/*
 * Optional standalone entry point.
 * Define AUTOEXECLIST_NO_MAIN when linking into cookie-init.
 */
#ifndef AUTOEXECLIST_NO_MAIN
int main(void)
{
    return autoexeclist_run(AUTOEXEC_LIST) == 0 ? 0 : 1;
}
#endif
