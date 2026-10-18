
#ifndef COOKIE_AUTOEXECLIST_H
#define COOKIE_AUTOEXECLIST_H

/*
 * Run programs listed in the specified file, one at a time.
 *
 * Returns:
 *   0  - The list was processed successfully.
 *  -1  - The list could not be opened or read.
 */
int autoexeclist_run(const char *list_path);

#endif /* COOKIE_AUTOEXECLIST_H */
