
#define _DEFAULT_SOURCE

#include "reboot.h"
#include "ss/jrnl/journal.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>

#define AUTOEXEC_LIST "/etc/cookie/autoexec.list"

static void mount_fs(const char *source,
                     const char *target,
                     const char *type,
                     unsigned long flags)
{
    if (mount(source, target, type, flags, NULL) == -1 &&
        errno != EBUSY) {
        fprintf(stderr, "cookie-init: mount %s failed: %s\n",
                target, strerror(errno));
    }
}

static void setup_filesystems(void)
{
    mount_fs("proc", "/proc", "proc", 0);
    mount_fs("sysfs", "/sys", "sysfs", 0);
    mount_fs("devtmpfs", "/dev", "devtmpfs", 0);
    mount_fs("devpts", "/dev/pts", "devpts",
             MS_NOSUID | MS_NOEXEC);
    mount_fs("tmpfs", "/run", "tmpfs",
             MS_NOSUID | MS_NODEV);
    mount_fs("tmpfs", "/tmp", "tmpfs",
             MS_NOSUID | MS_NODEV);
}

int main(void)
{
    if (getpid() != 1) {
        fprintf(stderr, "cookie-init: must run as PID 1\n");
        return 1;
    }

    if (journal_init("/var/log/cookie-init.log") == -1) {
        journal_log(JOURNAL_WARNING,
                    "Could not open journal file: %s",
                    strerror(errno));
    }

    journal_log(JOURNAL_INFO, "Cookie Init started as PID %ld",
                (long)getpid());

    /* Keep standard output and error visible on the console. */
    setenv("PATH",
           "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
           1);

    journal_log(JOURNAL_INFO, "Set $PATH");
    setenv("HOME", "/root", 1);
    journal_log(JOURNAL_INFO, "Set $HOME");
    setenv("SHELL", "/bin/sh", 1);
    journal_log(JOURNAL_INFO, "Set $SHELL");
    setenv("TERM", "linux", 0);
    journal_log(JOURNAL_INFO, "Set $TERM");

    if (sethostname("cookie-linux", 12) == -1) {
        fprintf(stderr, "cookie-init: sethostname failed: %s\n",
                strerror(errno));
    }
    journal_log(JOURNAL_INFO, "set hostname");

    /*
     * PID 1 should not accidentally terminate from a terminal
     * interrupt or a normal termination request.
     */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    fprintf(stderr, "\n");
    fprintf(stderr, "Cookie Linux Init\n");
    fprintf(stderr, "Starting as PID 1\n\n");

    setup_filesystems();

    fprintf(stderr, "cookie-init: starting autoexec supervisor\n");

    cookie_reboot_loop(AUTOEXEC_LIST);

    /* The supervisor should never return. */
    for (;;)
        pause();

    return 0;
}
