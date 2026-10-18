
#include "reboot.h"
#include "autoexeclist.h"

#include <stdio.h>
#include <unistd.h>

void cookie_reboot_loop(const char *list_path)
{
    for (;;) {
        fprintf(stderr, "cookie-init: starting autoexec list\n");

        if (autoexeclist_run(list_path) == -1) {
            fprintf(stderr,
                    "cookie-init: autoexec failed; retrying in 1 second\n");
            sleep(1);
        } else {
            fprintf(stderr,
                    "cookie-init: autoexec completed; restarting\n");
        }
    }
}
