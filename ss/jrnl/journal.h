
#ifndef COOKIE_JOURNAL_H
#define COOKIE_JOURNAL_H

enum journal_level {
    JOURNAL_DEBUG,
    JOURNAL_INFO,
    JOURNAL_WARNING,
    JOURNAL_ERROR
};

int journal_init(const char *path);
void journal_close(void);

void journal_log(enum journal_level level, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

int journal_clear(const char *path);

#endif
