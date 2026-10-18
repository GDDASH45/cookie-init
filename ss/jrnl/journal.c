
#define _POSIX_C_SOURCE 200809L

#include "journal.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE *journal_file;

static const char *level_name(enum journal_level level)
{
    switch (level) {
    case JOURNAL_DEBUG:
        return "DEBUG";
    case JOURNAL_INFO:
        return "INFO";
    case JOURNAL_WARNING:
        return "WARNING";
    case JOURNAL_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

int journal_init(const char *path)
{
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (journal_file != NULL)
        journal_close();

    journal_file = fopen(path, "a");

    if (journal_file == NULL)
        return -1;

    setvbuf(journal_file, NULL, _IOLBF, 0);

    journal_log(JOURNAL_INFO, "Cookie Init journal started");
    return 0;
}

void journal_close(void)
{
    if (journal_file != NULL) {
        fflush(journal_file);
        fclose(journal_file);
        journal_file = NULL;
    }
}

void journal_log(enum journal_level level, const char *format, ...)
{
    time_t now = time(NULL);
    struct tm tm_now;
    char timestamp[32] = "unknown-time";

    if (now != (time_t)-1 &&
        localtime_r(&now, &tm_now) != NULL) {
        if (strftime(timestamp, sizeof(timestamp),
                     "%Y-%m-%d %H:%M:%S", &tm_now) == 0) {
            strcpy(timestamp, "unknown-time");
        }
    }

    va_list args;

    fprintf(stderr, "[%s] [%s] ", timestamp, level_name(level));

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);

    if (journal_file != NULL) {
        fprintf(journal_file, "[%s] [%s] ", timestamp,
                level_name(level));

        va_start(args, format);
        vfprintf(journal_file, format, args);
        va_end(args);

        fputc('\n', journal_file);
        fflush(journal_file);
    }
}
